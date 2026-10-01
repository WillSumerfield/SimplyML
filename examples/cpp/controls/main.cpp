// Interactive controls steering a fake training loop on the main thread, which only reads the
// Control Store. Keys: Space pauses, R resets, O cycles the optimizer, Esc quits.
#include <simplyml/core/app.hpp>
#include <simplyml/ui/controls.hpp>
#include <simplyml/ui/key_bindings.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/stats.hpp>
#include <simplyml/ui/ui.hpp>

#include <chrono>
#include <cmath>
#include <random>
#include <thread>

int main()
{
    sml::AppConfig config;
    config.title          = "SimplyML - controls";
    config.escToQuit      = true;
    config.cameraControls = false;
    sml::App app{config};
    sml::Ui  ui{app};
    auto const& pal = ui.theme().palette;

    auto& root = ui.setRoot<sml::Row>();
    auto& left = root.add<sml::Column>();
    left.setExtent(sml::px(380));
    auto& controls = left.add<sml::ControlPanel>("Training", pal.accent);
    controls.setExtent(sml::fit());
    auto& pause = controls.addToggle("pause", "Paused");
    controls.addSlider("lr", "Learning rate", 1e-4, 1.0, 1e-2).setLog(true);
    controls.addSlider("noise", "Noise", 0.0, 0.5, 0.1);
    auto& opt = controls.addSelect("optimizer", "Optimizer", {"SGD", "Momentum", "Adam"}, 2);
    controls.add<sml::Select>("schedule", "Schedule", std::vector<std::string>{"Constant", "Cosine", "Step"}, 0,
                              sml::Select::Style::Radio);
    controls.addNumber("batch", "Batch size", 64).setRange(1, 4096).setInteger(true);
    auto& reset = controls.add<sml::Button>("reset", "Reset", pal.accent);
    left.add<sml::KeyBindings>("Keys").setExtent(sml::fit());

    auto& charts = root.add<sml::Column>();
    charts.add<sml::LineChart>("Loss", "loss", pal.blue).setValueFormat(sml::ValueFormat::number(4));
    auto& row = charts.add<sml::Row>();
    row.add<sml::LineChart>("Learning rate (effective)", "lr_eff", pal.yellow).setValueFormat(sml::ValueFormat::general(3));
    row.add<sml::StatCard>("Run", pal.teal).addBig("Step", "step");

    ui.bindKey(sf::Keyboard::Key::Space, pause);
    ui.bindKey(sf::Keyboard::Key::R, reset);
    ui.bindKey(sf::Keyboard::Key::O, opt, "Next optimizer");
    ui.bindKey(sf::Keyboard::Key::Escape, "Quit"); // handled by the App (escToQuit)

    app.start();
    std::mt19937                     rng{1};
    std::normal_distribution<double> noise{0.0, 1.0};
    auto&                            c       = app.controls();
    double                           loss    = 2.0;
    double                           lastReset = c.get("reset", 0);
    int                              step    = 0;
    while (app.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (double const r = c.get("reset", 0); r != lastReset) {
            lastReset = r;
            loss      = 2.0;
            step      = 0;
            app.store().clear();
        }
        if (c.get("pause", 0) > 0.5) {
            continue;
        }
        double const speed[] = {0.4, 0.8, 1.0};
        double lr = c.get("lr", 1e-2);
        if (c.get("schedule", 0) == 1) {
            lr *= 0.5 * (1.0 + std::cos(step * 0.003));
        } else if (c.get("schedule", 0) == 2) {
            lr *= std::pow(0.5, step / 1000);
        }
        double const batch = c.get("batch", 64);
        double const k     = speed[static_cast<int>(c.get("optimizer", 2))] * std::min(lr, 0.5);
        loss += -k * (loss - 0.05) + c.get("noise", 0.1) * std::sqrt(lr / batch) * 3.0 * noise(rng);
        if (lr > 0.3) {
            loss *= 1.0 + 0.02 * std::abs(noise(rng)); // too hot: diverges
        }
        app.store().push("loss", step, loss);
        app.store().push("lr_eff", step, lr);
        app.store().push("step", step, step);
        ++step;
    }
    app.join();
}
