#include "dashboard.hpp"

#include <cstdio>

#include <simplyml/ml/training_stats_card.hpp>
#include <simplyml/ui/bar_chart.hpp>
#include <simplyml/ui/controls.hpp>
#include <simplyml/ui/key_bindings.hpp>
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/phase_plot.hpp>
#include <simplyml/ui/stats.hpp>

#include "training/config.hpp"

Dashboard::Dashboard(sml::App& app, sml::Ui& ui, float scale, std::vector<std::string> inputLabels,
                     std::string outputTitle)
    : m_app{app}
    , m_ui{ui}
    , m_scale{scale}
    , m_labels{std::move(inputLabels)}
{
    ui.theme().scale = scale;
    auto& root = ui.setRoot<sml::Column>();
    root.setGap(0.0f);
    m_training = &root.add<sml::Row>();
    m_demo     = &root.add<sml::Row>();
    buildTraining(*m_training);
    buildDemo(*m_demo, outputTitle);
    m_demo->setVisible(false);

    // Keys from the original app; each one drives a control, so the panels and keys stay in sync.
    ui.bindKey(sf::Keyboard::Key::D, "Demo / training", "demo");
    ui.bindKey(sf::Keyboard::Key::A, "AI on/off (demo)", "ai");
    ui.bindKey(sf::Keyboard::Key::P, "Disturbances (demo)", "disturbance");
    ui.bindKey(sf::Keyboard::Key::B, "Best only (demo)", "best_only");
    ui.bindKey(sf::Keyboard::Key::S, "Fast demo", "fast");
    ui.bindKey(sf::Keyboard::Key::Space, "Next difficulty", "next");
    ui.bindKey(sf::Keyboard::Key::Backspace, "Lower friction", "easier");
    ui.bindKey(sf::Keyboard::Key::W, "Save all genomes", "dump");
    ui.bindKey(sf::Keyboard::Key::H, "Hide/show controls", [this] {
        m_controlsShown = !m_controlsShown;
        for (auto* w : m_controlWidgets) {
            w->setVisible(m_controlsShown);
        }
    });
    ui.bindKey(sf::Keyboard::Key::Escape, "Quit"); // App escToQuit
}

void Dashboard::addControls(sml::Column& column, bool legend)
{
    auto const& pal   = m_ui.theme().palette;
    auto&       panel = column.add<sml::ControlPanel>("Controls", pal.grey);
    panel.setExtent(sml::fit());
    panel.addToggle("demo", "Demo");
    panel.addToggle("ai", "AI", true);
    panel.addToggle("disturbance", "Disturbances");
    panel.addToggle("best_only", "Best only", true);
    panel.addToggle("fast", "Fast demo");
    panel.addButton("next", "Next difficulty");
    panel.addButton("easier", "Lower friction");
    panel.addButton("dump", "Save genomes");
    m_controlWidgets.push_back(&panel);
    if (legend) {
        auto& keys = column.add<sml::KeyBindings>("Keys", pal.grey);
        keys.setExtent(sml::fit());
        m_controlWidgets.push_back(&keys);
    }
}

void Dashboard::buildTraining(sml::Row& view)
{
    auto const& pal = m_ui.theme().palette;
    sf::Color const hud{100, 170, 255};

    auto& left = view.add<sml::Column>();
    left.setExtent(px(380));
    auto& tileRow = left.add<sml::Row>();
    tileRow.setExtent(px(70));
    tileRow.add<sml::StatTile>("Generation", "iteration", sml::ValueFormat::integer(4), pal.teal).setExtent(px(290));
    addControls(left, true);

    auto& center = view.add<sml::Column>();
    center.add<sml::Column>().setExtent(px(90)); // the original's score card starts at y = 130
    auto& scoreRow = center.add<sml::Row>();
    scoreRow.setAlign(sml::Align::Center).setExtent(px(489));
    scoreRow.add<sml::BarChart>("Score", "best_score", hud).setWindow(200).setFill(true)
        .setValueFormat(sml::ValueFormat::number(5)).setExtent(px(1200));
    auto& plots = center.add<sml::Row>();
    plots.setAlign(sml::Align::Center).setExtent(px(326));
    plots.add<sml::LineChart>("Gravity", "gravity", pal.yellow).setWindow(200)
        .setValueFormat(sml::ValueFormat::number(2)).setExtent(px(800));
    plots.add<sml::LineChart>("Friction", "friction", pal.accent).setWindow(200)
        .setValueFormat(sml::ValueFormat::number(10)).setExtent(px(800));
    auto& netRow = center.add<sml::Row>();
    netRow.setAlign(sml::Align::Center);
    auto& netCol = netRow.add<sml::Column>();
    netCol.setExtent(sml::fit());
    m_trainingNet = &netCol.add<sml::NetworkView>("", sf::Color::White);
    m_trainingNet->setExtent(sml::fit());

    view.add<sml::Column>().setExtent(px(380)); // keeps the center column centered
}

void Dashboard::buildDemo(sml::Row& view, std::string const& outputTitle)
{
    auto const& pal = m_ui.theme().palette;
    float const side = 305.0f;

    auto& left = view.add<sml::Column>();
    left.setExtent(px(side));
    auto& sceneCard = left.add<sml::StatCard>("", sf::Color::White);
    sceneCard.setExtent(sml::fit());
    sceneCard.addBig("", "demo_time", sml::ValueFormat::number(1, "s"));
    sceneCard.addStatus("AI state", "ai_state");
    sceneCard.addStatus("Disturbance", "disturbance_state");

    auto& iteration = left.add<sml::TrainingStatsCard>("Iteration", sf::Color::White);
    iteration.setExtent(sml::fit());
    iteration.bestScore().setVisible(false);
    auto& gravity = iteration.addGauge("Gravity (/1000)", "gravity", 0.0, conf::sim::max_gravity, pal.yellow);
    gravity.setFormat(sml::ValueFormat::number(0));
    auto& friction = iteration.addGauge("Friction", "friction", 0.0, 0.003, pal.accent);
    friction.setFormat(sml::ValueFormat::number(5));
    iteration.rows().move(gravity, 1);
    iteration.rows().move(friction, 2);
    addControls(left, false);

    auto& center = view.add<sml::Column>();
    m_scene = &center.add<sml::CartPendulumView>("", pal.accent);
    m_scene->setExtent(px(950));
    float const cx = conf::sim::world_size.x * 0.5f, cy = conf::sim::world_size.y * 0.5f;
    m_scene->setRail(cx - 0.5f * conf::sim::slider_length, cx + 0.5f * conf::sim::slider_length, cy)
        .setWorld({{-25.0f, -25.0f}, {conf::sim::world_size.x + 50.0f, conf::sim::world_size.y + 50.0f}});
    if (sf::Texture const* wheel = m_app.resources().findTexture("wheel")) {
        m_scene->setWheelTexture(wheel);
    }
    auto& bottom = center.add<sml::Column>();
    bottom.setAlign(sml::Align::Center);
    auto& hud = bottom.add<sml::Row>();
    hud.setAlign(sml::Align::Center).setExtent(px(369));
    auto& netCol = hud.add<sml::Column>();
    netCol.setExtent(sml::fit());
    m_demoNet = &netCol.add<sml::NetworkView>("", sf::Color::White);
    auto& output = hud.add<sml::LineChart>(outputTitle, "output", pal.yellow);
    output.setWindow(200).setXFormat(sml::ValueFormat::number(1)).setExtent(px(2.25f * 369.0f));
    sml::ValueFormat speed;
    speed.custom = [](double v) { // the original shows output * max_accel / 100 in m/s
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f m/s", v * 0.01);
        return std::string(buf);
    };
    output.setValueFormat(speed);

    auto& right = view.add<sml::Column>();
    right.setExtent(px(side));
    auto deg = sml::ValueFormat::number(1, " deg");
    right.add<sml::LineChart>("Angle base", "angle_base", pal.green).setWindow(100)
        .setXFormat(sml::ValueFormat::number(1)).setValueFormat(deg).setExtent(px(side));
    right.add<sml::LineChart>("Angle mid", "angle_mid", pal.orange).setWindow(100)
        .setXFormat(sml::ValueFormat::number(1)).setValueFormat(deg).setExtent(px(side));
    right.add<sml::PhasePlot>("Angle base vs mid", "angle_base", "angle_mid", pal.accent).setMaxPoints(300)
        .setExtent(px(side));
}

void Dashboard::update()
{
    bool const demo = m_app.controls().get("demo", 0.0) > 0.5;
    if (demo != m_demo->visible()) {
        std::lock_guard lock{m_ui.mutex()};
        m_demo->setVisible(demo);
        m_training->setVisible(!demo);
    }
}
