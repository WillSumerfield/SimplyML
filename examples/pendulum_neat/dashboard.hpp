#pragma once
#include <string>
#include <vector>

#include <simplyml/core/app.hpp>
#include <simplyml/ml/cart_pendulum_view.hpp>
#include <simplyml/ml/network_view.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/ui.hpp>

/// The two screens of the original app, built from SimplyML widgets:
/// - training: generation tile, score bars, gravity/friction charts and the best network;
/// - demo: the best agent (and ghosts) on the rail, its live network, output and angle charts.
/// The trainer only pushes series, sets control values and hands formats to the views; the
/// screen shown follows the "demo" control.
class Dashboard
{
public:
    /// Original layout sizes are for 2560x1440; `scale` shrinks them (and the theme) to the window.
    Dashboard(sml::App& app, sml::Ui& ui, float scale, std::vector<std::string> inputLabels, std::string outputTitle);

    /// UI thread, once per frame: shows the screen matching the "demo" control.
    void update();

    // Thread-safe sinks for the trainer.
    sml::NetworkView&      trainingNetwork() { return *m_trainingNet; }
    sml::NetworkView&      demoNetwork()     { return *m_demoNet; }
    sml::CartPendulumView& scene()           { return *m_scene; }
    [[nodiscard]] std::vector<std::string> const& inputLabels() const { return m_labels; }

private:
    void buildTraining(sml::Row& view);
    void buildDemo(sml::Row& view, std::string const& outputTitle);
    void addControls(sml::Column& column, bool legend);
    [[nodiscard]] sml::Length px(float v) const { return sml::px(v * m_scale); }

    sml::App&                m_app;
    sml::Ui&                 m_ui;
    float                    m_scale;
    std::vector<std::string> m_labels;
    sml::Row*                m_training = nullptr;
    sml::Row*                m_demo     = nullptr;
    std::vector<sml::Widget*> m_controlWidgets; // hidden with H
    sml::NetworkView*        m_trainingNet = nullptr;
    sml::NetworkView*        m_demoNet     = nullptr;
    sml::CartPendulumView*   m_scene       = nullptr;
    bool                     m_controlsShown = true;
};
