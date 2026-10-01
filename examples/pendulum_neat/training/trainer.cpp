#include "training/trainer.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include <simplyml/ml/formats.hpp>
#include <simplyml/util/math.hpp>

#include "common/binary_io.hpp"
#include "dashboard.hpp"

namespace
{

using Clock = std::chrono::steady_clock;

constexpr float Dt = 1.0f / 60.0f; // the original's fixed step, for training and demo

/// NEAT network -> LayeredGraph. Layers are node depths; non-input nodes still at depth 0 (no
/// incoming connection yet) go one column right of the inputs, as in the original renderer.
/// `live` shows the last evaluation's values (inputs: their sum; others: activation; edges: the
/// signal they carried); otherwise everything is 0 (plain white drawing).
sml::LayeredGraph toGraph(nt::Network const& nw, std::vector<std::string> const& labels, bool live)
{
    sml::LayeredGraph g;
    uint32_t const    n = nw.info.getNodeCount();
    g.nodes.resize(n);
    for (uint32_t i{0}; i < n; ++i) {
        auto const& node = nw.getNode(i);
        auto&       out  = g.nodes[i];
        out.layer = static_cast<int>(node.depth);
        if (i >= nw.info.inputs && node.depth == 0) {
            out.layer = 1;
        }
        if (live) {
            out.value = static_cast<float>(i < nw.info.inputs ? node.sum : node.getValue());
        }
        if (i < nw.info.inputs && i < labels.size()) {
            out.label = labels[i];
        }
    }
    uint32_t c{0};
    for (uint32_t i{0}; i < n; ++i) {
        for (uint32_t k{0}; k < nw.getNode(i).connection_count; ++k, ++c) {
            auto const& con = nw.getConnection(c);
            g.edges.push_back({static_cast<int>(i), static_cast<int>(con.to), live ? static_cast<float>(con.value) : 0.0f});
        }
    }
    return g;
}

sml::LinkChainState chainOf(training::Scene const& s, bool withPush)
{
    sml::LinkChainState out;
    out.base = sf::Vector2f(s.agent.system.drag_constraints[0].target);
    for (auto const& o : s.agent.system.objects) {
        out.joints.emplace_back(o.getWorldPosition(1));
    }
    if (withPush) {
        Disturbances::Push const& push = s.getCurrentPush();
        if (s.isActive(push)) {
            out.push = static_cast<float>(push.force);
        }
    }
    return out;
}

} // namespace

Trainer::Trainer(sml::App& app, Dashboard& dashboard, std::string outputDir, uint32_t threads)
    : m_app{app}
    , m_dash{dashboard}
    , m_outputDir{std::move(outputDir)}
    , agents(conf::sel::population_size)
    , sequences(2)
    , pool{threads}
    , evolver{state, agents}
{
    /* One scene per agent, forever linked to it. Score: stay up, near the center, with a calm
       output. */
    scenes.reserve(conf::sel::population_size);
    for (uint32_t i{0}; i < conf::sel::population_size; ++i) {
        auto& task              = scenes.emplace_back(agents[i], sequences, state, 1);
        task.enable_disturbance = false;
        task.freeze_time        = 0.0;
        task.score_function     = [](pbd::RealType pos_x, pbd::RealType out_sum, pbd::RealType) {
            pbd::RealType const dist_to_center_penalty = std::abs(1.0 - std::abs(pos_x));
            return 1.0 / (1.0 + out_sum * 0.5) * dist_to_center_penalty;
        };
    }
    sequences[0].generateSequence();
    sequences[1].generateSequence();

    auto& store = m_app.store();
    for (char const* s : {"demo_time", "ai_state", "disturbance_state"}) {
        store.setMaxPoints(s, 2);
    }
    for (char const* s : {"output", "angle_base", "angle_mid"}) {
        store.setMaxPoints(s, 400);
    }
    restartExploration();
}

void Trainer::run()
{
    auto next = Clock::now();
    while (m_app.isRunning()) {
        handleControls();
        if (!demo) {
            trainIteration();
            publishIteration();
            next = Clock::now();
            continue;
        }
        demoStep(Dt);
        bool const fast = control("fast");
        auto const now  = Clock::now();
        if (fast) {
            if (now >= next) { // publish at most 60 times a second
                publishDemoFrame();
                next = now + std::chrono::microseconds(16667);
            }
        } else {
            publishDemoFrame();
            next += std::chrono::microseconds(16667);
            if (next < now - std::chrono::milliseconds(100)) {
                next = now; // fell behind (e.g. slow machine): don't try to catch up
            }
            std::this_thread::sleep_until(next);
        }
    }
}

// Controls -----------------------------------------------------------------------------------------

bool Trainer::control(char const* name) const
{
    double const fallback = std::string_view{name} == "ai" || std::string_view{name} == "best_only" ? 1.0 : 0.0;
    return m_app.controls().get(name, fallback) > 0.5;
}

bool Trainer::clicked(char const* name, double& seen) const
{
    double const v = m_app.controls().get(name, 0.0);
    if (v == seen) {
        return false;
    }
    seen = v;
    return true;
}

void Trainer::handleControls()
{
    if (control("demo") != demo) {
        if (demo) {
            endDemo();
        } else {
            startDemo();
        }
    }
    if (demo) {
        scenes[0].enable_ai = control("ai");
        bool const disturbance = control("disturbance");
        if (disturbance != scenes[0].enable_disturbance) {
            for (auto& s : scenes) {
                s.enable_disturbance = disturbance;
            }
        }
    }
    if (clicked("next", seenNext)) {
        bypass_score_threshold = true;
    }
    if (clicked("easier", seenEasier)) {
        bypass_score_threshold = true;
        state.configuration.solver_friction *= 0.95f;
    }
    if (clicked("dump", seenDump)) {
        try {
            writeAllGenomes();
        } catch (std::exception const& e) {
            std::cout << "Couldn't save genomes: " << e.what() << std::endl;
        }
    }
}

// Stadium ------------------------------------------------------------------------------------------

void Trainer::trainIteration()
{
    auto const start = Clock::now();
    // Update state, run all tasks, then create the next generation
    state.addIteration();
    executeTasks(Dt);
    evolver.createNewGeneration();
    state.iteration_best_score = agents[0].score;
    // Check if we need to make the task harder
    if (needIncreaseDifficulty()) {
        increaseDifficulty();
    } else if (state.iteration % 10 == 0) {
        saveBest(true);
    }
    trainingSeconds += std::chrono::duration<double>(Clock::now() - start).count();
}

void Trainer::publishIteration()
{
    sml::TrainingStats stats;
    stats.iteration = state.iteration;
    stats.bestScore = agents[0].score;
    stats.simTime   = double(state.iteration) * conf::sel::population_size * conf::sel::max_iteration_time;
    stats.wallTime  = trainingSeconds;
    stats.extra     = {{"gravity", state.configuration.solver_gravity}, {"friction", state.configuration.solver_friction}};
    sml::pushStats(m_app.store(), stats);
    m_dash.trainingNetwork().setGraph(toGraph(agents[0].generateNetwork(), m_dash.inputLabels(), false));
}

void Trainer::initializeIteration()
{
    // Only change the training sequence
    sequences[1].generateSequence();
    pool.dispatch(static_cast<uint32_t>(scenes.size()), [&](uint32_t start, uint32_t end) {
        for (uint32_t i{start}; i < end; ++i) {
            scenes[i].push_sequence_id = 1;
            scenes[i].initialize();
        }
    });
}

void Trainer::executeTasks(float dt)
{
    initializeIteration();
    pool.dispatch(static_cast<uint32_t>(scenes.size()), [&](uint32_t start, uint32_t end) {
        float t = 0.0f;
        while (t < conf::sel::max_iteration_time) {
            bool done = true;
            for (uint32_t i{start}; i < end; ++i) {
                if (!scenes[i].done()) {
                    scenes[i].update(dt);
                    done = false;
                }
            }
            if (done) {
                break;
            }
            t += dt;
        }
    });
}

std::string Trainer::currentFolder() const
{
    return m_outputDir + "/genomes_" + std::to_string(state.iteration_exploration);
}

void Trainer::saveBest(bool force) const
{
    if (((state.iteration % conf::exp::best_save_period) == 0) || force) {
        agents[0].genome.writeToFile(currentFolder() + "/best_" + std::to_string(state.iteration) + ".bin");
        saveConfiguration(currentFolder() + "/best_conf_" + std::to_string(state.iteration) + ".bin");
    }
}

void Trainer::saveConfiguration(std::string const& filename) const
{
    BinaryWriter conf_writer{filename};
    conf_writer.write(state.configuration.max_speed);
    conf_writer.write(state.configuration.max_accel);
    conf_writer.write(state.configuration.solver_friction);
    conf_writer.write(state.configuration.solver_gravity);
    conf_writer.write(state.configuration.solver_sub_steps);
    conf_writer.write(state.configuration.solver_compliance);
    conf_writer.write(state.configuration.task_sub_steps);
}

void Trainer::writeAllGenomes() const
{
    std::string const path_prefix = currentFolder() + "/dump_" + std::to_string(state.iteration);
    std::filesystem::create_directories(path_prefix);
    for (uint32_t i{0}; i < agents.size(); ++i) {
        agents[i].genome.writeToFile(path_prefix + "/genome_" + std::to_string(i) + ".bin");
    }
    saveConfiguration(path_prefix + "/configuration.bin");
    std::cout << "Genomes saved to " << path_prefix << std::endl;
}

bool Trainer::needIncreaseDifficulty() const
{
    return (state.iteration_best_score > target_score) || bypass_score_threshold;
}

void Trainer::increaseDifficulty()
{
    bypass_score_threshold = false;
    saveBest(true);
    if (state.configuration.solver_friction > 0.0f) {
        state.configuration.solver_friction -= 0.000001;
    } else {
        state.configuration.solver_friction = 0.0f;
    }
    if (state.configuration.solver_gravity < conf::sim::max_gravity) {
        state.configuration.solver_gravity *= 1.01f;
        if (state.configuration.solver_gravity > conf::sim::max_gravity) {
            state.configuration.solver_gravity = conf::sim::max_gravity;
        }
    }
    std::cout << "Gravity: " << state.configuration.solver_gravity
              << " Friction: " << state.configuration.solver_friction << std::endl;
}

void Trainer::restartExploration()
{
    state.newExploration();
    RNGf::setSeed(state.iteration_exploration + conf::exp::seed_offset);
    std::filesystem::create_directories(currentFolder());
    // Base genome is the last exploration's best
    auto const best_genome = agents[0].genome;
    for (auto& a : agents) {
        a.genome = best_genome;
    }
}

void Trainer::loadGenome(std::string const& filename)
{
    nt::Genome genome;
    genome.loadFromFile(filename);
    for (auto& a : agents) {
        a.genome = genome;
    }
}

void Trainer::loadConf(std::string const& filename)
{
    BinaryReader conf_reader{filename};
    conf_reader.readInto(state.configuration.max_speed);
    conf_reader.readInto(state.configuration.max_accel);
    conf_reader.readInto(state.configuration.solver_friction);
    conf_reader.readInto(state.configuration.solver_gravity);
    conf_reader.readInto(state.configuration.solver_sub_steps);
    conf_reader.readInto(state.configuration.solver_compliance);
    conf_reader.readInto(state.configuration.task_sub_steps);
    std::cout << "[Conf loaded] gravity " << state.configuration.solver_gravity << ", friction "
              << state.configuration.solver_friction << std::endl;
}

// Demo ---------------------------------------------------------------------------------------------

void Trainer::startDemo()
{
    demo = true;
    // Every scene replays the reference push sequence, after a one second freeze
    pool.dispatch(static_cast<uint32_t>(scenes.size()), [&](uint32_t start, uint32_t end) {
        for (uint32_t i{start}; i < end; ++i) {
            scenes[i].push_sequence_id   = 0;
            scenes[i].enable_disturbance = false;
            scenes[i].initialize();
            scenes[i].freeze_time = 1.0f;
        }
    });
    auto& c = m_app.controls();
    c.set("ai", 1.0);
    c.set("disturbance", 0.0);
    for (char const* s : {"output", "angle_base", "angle_mid"}) {
        m_app.store().clear(s);
    }
    publishDemoFrame();
}

void Trainer::endDemo()
{
    demo = false;
    std::cout << "Demo score: " << scenes[0].getAgentInfo().score << std::endl;
    for (auto& s : scenes) {
        s.push_sequence_id   = 1;
        s.freeze_time        = 0.0f;
        s.enable_disturbance = false;
    }
}

void Trainer::demoStep(float dt)
{
    pool.dispatch(static_cast<uint32_t>(scenes.size()), [&](uint32_t start, uint32_t end) {
        for (uint32_t i{start}; i < end; ++i) {
            if (!scenes[i].done()) {
                scenes[i].update(dt);
            }
        }
    });
    auto const& best   = scenes[0];
    auto const& system = best.agent.system;
    auto&       store  = m_app.store();
    double const t     = best.current_time;
    store.push("output", t, best.network.output[0] * best.configuration.max_accel * 0.01);
    double const a1 = sml::radToDeg(system.objects[0].angle);
    double const a2 = sml::radToDeg(system.objects[1].angle);
    store.push("angle_base", t, 270.0 - a1);
    store.push("angle_mid", t, a1 - a2);
}

void Trainer::publishDemoFrame()
{
    auto const& best  = scenes[0];
    auto&       store = m_app.store();
    store.push("demo_time", best.current_time - best.freeze_time);
    store.push("ai_state", best.enable_ai ? 1.0 : 0.0);
    store.push("disturbance_state", best.enable_disturbance ? 1.0 : 0.0);

    m_dash.scene().setState(chainOf(best, true));
    std::vector<sml::LinkChainState> ghosts;
    if (!control("best_only")) {
        ghosts.reserve(scenes.size() - 1);
        for (std::size_t i = 1; i < scenes.size(); ++i) {
            ghosts.push_back(chainOf(scenes[i], false));
        }
    }
    m_dash.scene().setGhosts(std::move(ghosts));
    m_dash.demoNetwork().setGraph(toGraph(best.network, m_dash.inputLabels(), true));
}
