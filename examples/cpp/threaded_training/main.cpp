// The UI runs on a background thread while the main thread "trains" and pushes metrics.
// Close the window (or wait for training to finish) to exit.
#include <simplyml/core/app.hpp>
#include <simplyml/core/snapshot.hpp>
#include <simplyml/util/format.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <chrono>
#include <cmath>
#include <random>
#include <thread>

namespace
{

struct Stats
{
    int    epoch = 0;
    double best  = 1e9;
};

// Crude line plot of one series, in screen space (real charts arrive with the UI layer).
void plot(sml::Canvas& canvas, sml::Series const& s, sf::FloatRect box, sf::Color color)
{
    if (s.size() < 2) {
        return;
    }
    double lo = s.points()[0].value;
    double hi = lo;
    for (auto const& p : s.points()) {
        lo = std::min(lo, p.value);
        hi = std::max(hi, p.value);
    }
    double const range = hi > lo ? hi - lo : 1.0;
    double const x0    = s.points().front().step;
    double const xr    = std::max(s.last().step - x0, 1e-9);

    sf::VertexArray line{sf::PrimitiveType::LineStrip, s.size()};
    for (std::size_t i = 0; i < s.size(); ++i) {
        auto const& p = s.points()[i];
        line[i].position = {box.position.x + static_cast<float>((p.step - x0) / xr) * box.size.x,
                            box.position.y + box.size.y - static_cast<float>((p.value - lo) / range) * box.size.y};
        line[i].color = color;
    }
    canvas.drawScreen(line);
}

} // namespace

int main()
{
    sml::AppConfig config;
    config.title     = "SimplyML - threaded training";
    config.size      = {1200, 700};
    config.escToQuit = true;
    sml::App app{config};
    sml::Snapshot<Stats> stats;

    app.onDraw([&](sml::Canvas& canvas) {
        auto const& font = app.resources().defaultFont();
        auto const  size = canvas.size();
        sf::FloatRect const box{{40.0f, 80.0f}, {size.x - 80.0f, size.y - 120.0f}};
        if (auto const* loss = app.store().series("loss")) {
            plot(canvas, *loss, box, {231, 111, 81});
        }
        if (auto const* acc = app.store().series("acc")) {
            plot(canvas, *acc, box, {42, 157, 143});
        }
        Stats const& st = stats.get();
        sf::Text text{font, "epoch " + std::to_string(st.epoch) + "   best loss " + sml::toString(st.best, 4) +
                                "   fps " + sml::toString(1.0f / std::max(app.clock().dt(), 1e-6f), 0),
                      24};
        text.setPosition({40.0f, 30.0f});
        canvas.drawScreen(text);
    });
    app.start();

    std::mt19937                     rng{42};
    std::normal_distribution<double> noise{0.0, 0.03};
    for (int epoch = 0; epoch < 2000 && app.isRunning(); ++epoch) {
        double const loss = std::exp(-epoch / 300.0) + noise(rng);
        app.store().push("loss", epoch, loss);
        app.store().push("acc", epoch, 1.0 - std::exp(-epoch / 400.0) + noise(rng));
        stats.update([&](Stats& s) {
            s.epoch = epoch;
            s.best  = std::min(s.best, loss);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // "training"
    }
    app.close();
    app.join();
}
