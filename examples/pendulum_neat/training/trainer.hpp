#pragma once
#include <string>
#include <vector>

#include <simplyml/core/app.hpp>

#include "common/thread_pool.hpp"
#include "training/agent_info.hpp"
#include "training/disturbances.hpp"
#include "training/evolver.hpp"
#include "training/scene.hpp"
#include "training/training_state.hpp"

class Dashboard;

/// The original Stadium (NEAT training with growing difficulty) and Demo (watching the population)
/// processors, without the ECS. Runs on its own thread (`run`); talks to the UI only through the
/// Metric Store, the Control Store and the views' thread-safe setters.
class Trainer
{
public:
    Trainer(sml::App& app, Dashboard& dashboard, std::string outputDir, uint32_t threads);

    /// Trains (or runs the demo) until the App closes.
    void run();

    /// Loads a genome (and, if present, its configuration file) into every agent.
    void loadGenome(std::string const& filename);
    void loadConf(std::string const& filename);

private:
    // Stadium
    void restartExploration();
    void trainIteration();
    void initializeIteration();
    void executeTasks(float dt);
    void saveBest(bool force = false) const;
    void saveConfiguration(std::string const& filename) const;
    void writeAllGenomes() const;
    [[nodiscard]] bool needIncreaseDifficulty() const;
    void increaseDifficulty();
    [[nodiscard]] std::string currentFolder() const;

    // Demo
    void startDemo();
    void endDemo();
    void demoStep(float dt);

    // UI
    void handleControls();
    void publishIteration();
    void publishDemoFrame();
    [[nodiscard]] bool control(char const* name) const;
    /// True once per click of a button control.
    bool clicked(char const* name, double& seen) const;

    sml::App&   m_app;
    Dashboard&  m_dash;
    std::string m_outputDir;

    TrainingState                state;
    std::vector<AgentInfo>       agents;
    std::vector<Disturbances>    sequences; // 0 = reference (demo), 1 = training, regenerated each iteration
    std::vector<training::Scene> scenes;
    tp::ThreadPool               pool;
    Evolver                      evolver;

    float  target_score           = 8.0f;
    bool   bypass_score_threshold = false;
    double trainingSeconds        = 0.0; // real time spent training

    bool   demo = false;
    double seenNext = 0.0, seenEasier = 0.0, seenDump = 0.0;
};
