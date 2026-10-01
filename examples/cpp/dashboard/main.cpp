// Every display widget, fed by a fake training loop on the main thread. Esc quits.
#include <simplyml/core/app.hpp>
#include <simplyml/ui/bar_chart.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/phase_plot.hpp>
#include <simplyml/ui/stats.hpp>
#include <simplyml/ui/ui.hpp>

#include <chrono>
#include <cmath>
#include <random>
#include <thread>

int main()
{
    sml::AppConfig config;
    config.title          = "SimplyML - dashboard";
    config.size           = {1600, 900};
    config.escToQuit      = true;
    config.cameraControls = false;
    sml::App app{config};
    sml::Ui  ui{app};
    auto const& pal = ui.theme().palette;

    auto& root = ui.setRoot<sml::Row>();

    // Left: stats column.
    auto& left = root.add<sml::Column>();
    left.setExtent(sml::px(340));
    left.add<sml::StatTile>("Epoch", "epoch", sml::ValueFormat::integer(4), pal.teal).setExtent(sml::fit());
    auto& stats = left.add<sml::StatCard>("Training", pal.blue);
    stats.addBig("Best loss", "best_loss", sml::ValueFormat::number(4));
    stats.addGauge("Learning rate", "lr", 0.0, 0.01, pal.yellow).setFormat(sml::ValueFormat::number(5));
    stats.addGauge("Progress", "progress", 0.0, 1.0, pal.accent).setFormat(sml::ValueFormat::percent());
    stats.addValue("Wall time", "wall", sml::ValueFormat::duration());
    stats.addStatus("Augmentation", "augment");
    stats.addStatus("Early stopping", "early_stop", "Armed", "Off");

    // Right: charts.
    auto& grid = root.add<sml::Grid>(2);
    grid.setGap(-1);
    auto& loss = grid.add<sml::LineChart>("Loss");
    loss.addSeries("train_loss", pal.accent, "train").addSeries("val_loss", pal.blue, "val");
    loss.setSpan(2);
    grid.add<sml::LineChart>("Accuracy", "acc", pal.teal).setWindow(300);
    grid.add<sml::BarChart>("Score / epoch", "score", pal.blue).setWindow(40);
    grid.add<sml::PhasePlot>("Phase", "theta", "omega", pal.orange);
    grid.add<sml::LineChart>("Gradient norm", "grad", pal.yellow).setAutoY(false);

    app.start();
    std::mt19937                    rng{1};
    std::normal_distribution<double> noise{0.0, 1.0};
    auto&                           s     = app.store();
    auto const                      start = std::chrono::steady_clock::now();
    double                          best  = 1e9;
    for (int step = 0; app.isRunning(); ++step) {
        double const t     = step * 0.01;
        double const train = 2.0 * std::exp(-step / 400.0) + 0.05 + 0.03 * noise(rng);
        double const val   = 2.1 * std::exp(-step / 450.0) + 0.09 + 0.04 * noise(rng);
        best = std::min(best, val);
        s.push("train_loss", step, train);
        s.push("val_loss", step, val);
        s.push("acc", step, 1.0 - 0.9 * std::exp(-step / 300.0) + 0.01 * noise(rng));
        s.push("grad", step, 0.5 + 0.4 * std::sin(t) + 0.05 * noise(rng));
        s.push("theta", std::sin(t * 2.0) * std::exp(-t / 30.0));
        s.push("omega", 2.0 * std::cos(t * 2.0) * std::exp(-t / 30.0));
        if (step % 50 == 0) {
            s.push("epoch", step / 50);
            s.push("score", step / 50, 100.0 * (1.0 - std::exp(-step / 800.0)) + 5.0 * noise(rng));
            s.push("best_loss", best);
            s.push("lr", 0.008 * std::pow(0.97, step / 50));
            s.push("progress", std::min(1.0, step / 5000.0));
            s.push("augment", (step / 500) % 2);
            s.push("early_stop", step > 1500 ? 1.0 : 0.0);
            s.push("wall", std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() * 3600.0);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    app.join();
}
