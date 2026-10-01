// A small MLP (2-6-6-1, tanh) learning to tell points inside a ring from points outside, drawn
// live with NetworkView: node fill = activations for a probe input, edge width/color = weights.
// Training runs on the main thread; the window on its own. Esc quits.
#include <simplyml/core/app.hpp>
#include <simplyml/ml/network_view.hpp>
#include <simplyml/ml/training_stats_card.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/ui.hpp>

#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <vector>

namespace
{

struct Mlp
{
    std::vector<int>                              sizes;
    std::vector<std::vector<float>>               w, b; // w[l][o * in + i]
    std::vector<std::vector<float>>               a;    // activations per layer

    Mlp(std::vector<int> s, std::mt19937& rng)
        : sizes{std::move(s)}
    {
        for (std::size_t l = 0; l + 1 < sizes.size(); ++l) {
            std::normal_distribution<float> init{0.0f, 1.0f / std::sqrt(float(sizes[l]))};
            w.emplace_back(std::size_t(sizes[l] * sizes[l + 1]));
            for (auto& x : w.back()) {
                x = init(rng);
            }
            b.emplace_back(std::size_t(sizes[l + 1]), 0.0f);
        }
        for (int n : sizes) {
            a.emplace_back(std::size_t(n), 0.0f);
        }
    }

    float forward(float x, float y)
    {
        a[0] = {x, y};
        for (std::size_t l = 0; l + 1 < sizes.size(); ++l) {
            int const in = sizes[l];
            for (int o = 0; o < sizes[l + 1]; ++o) {
                float s = b[l][std::size_t(o)];
                for (int i = 0; i < in; ++i) {
                    s += w[l][std::size_t(o * in + i)] * a[l][std::size_t(i)];
                }
                a[l + 1][std::size_t(o)] = std::tanh(s);
            }
        }
        return a.back()[0];
    }

    // One SGD step on a squared error; returns the loss.
    float train(float x, float y, float target, float lr)
    {
        float const out = forward(x, y);
        std::vector<float> delta{(out - target) * (1.0f - out * out)};
        for (std::size_t l = sizes.size() - 1; l-- > 0;) {
            int const in = sizes[l];
            std::vector<float> prev(std::size_t(in), 0.0f);
            for (int o = 0; o < sizes[l + 1]; ++o) {
                for (int i = 0; i < in; ++i) {
                    prev[std::size_t(i)] += w[l][std::size_t(o * in + i)] * delta[std::size_t(o)];
                    w[l][std::size_t(o * in + i)] -= lr * delta[std::size_t(o)] * a[l][std::size_t(i)];
                }
                b[l][std::size_t(o)] -= lr * delta[std::size_t(o)];
            }
            for (int i = 0; i < in; ++i) {
                prev[std::size_t(i)] *= 1.0f - a[l][std::size_t(i)] * a[l][std::size_t(i)];
            }
            delta = std::move(prev);
        }
        return 0.5f * (out - target) * (out - target);
    }

    [[nodiscard]] sml::LayeredGraph graph() const
    {
        sml::LayeredGraph g;
        std::vector<int>  first;
        for (std::size_t l = 0; l < sizes.size(); ++l) {
            first.push_back(int(g.nodes.size()));
            for (int i = 0; i < sizes[l]; ++i) {
                std::string label = l == 0 ? (i == 0 ? "x" : "y") : l + 1 == sizes.size() ? "inside" : "";
                g.nodes.push_back({int(l), a[l][std::size_t(i)], std::move(label)});
            }
        }
        for (std::size_t l = 0; l + 1 < sizes.size(); ++l) {
            for (int o = 0; o < sizes[l + 1]; ++o) {
                for (int i = 0; i < sizes[l]; ++i) {
                    g.edges.push_back({first[l] + i, first[l + 1] + o, w[l][std::size_t(o * sizes[l] + i)]});
                }
            }
        }
        return g;
    }
};

} // namespace

int main()
{
    sml::AppConfig config;
    config.title          = "SimplyML - network view";
    config.escToQuit      = true;
    config.cameraControls = false;
    sml::App app{config};
    sml::Ui  ui{app};
    auto const& pal = ui.theme().palette;

    auto& root = ui.setRoot<sml::Row>();
    auto& left = root.add<sml::Column>();
    left.setExtent(sml::px(420));
    auto& stats = left.add<sml::TrainingStatsCard>("Epoch", pal.teal);
    stats.setExtent(sml::fit());
    stats.addGauge("Accuracy", "acc", 0.0, 1.0, pal.green).setFormat(sml::ValueFormat::percent());
    stats.rows().move(stats.bestScore(), 99);
    stats.bestScore().setLabel("Best loss");
    auto& net = left.add<sml::NetworkView>("Network", sf::Color::White);
    net.setEdgeScale(1.5f).setMaxZoom(2.0f);

    auto& charts = root.add<sml::Column>();
    charts.add<sml::LineChart>("Loss", "loss", pal.accent).setValueFormat(sml::ValueFormat::number(4));
    charts.add<sml::LineChart>("Accuracy", "acc", pal.green).setYRange(0.0, 1.0);

    app.start();
    std::mt19937                          rng{3};
    std::uniform_real_distribution<float> u{-1.0f, 1.0f};
    Mlp                                   mlp{{2, 6, 6, 1}, rng};
    auto const label = [](float x, float y) { float const r = std::hypot(x, y); return r > 0.35f && r < 0.75f ? 1.0f : -1.0f; };
    auto const start = std::chrono::steady_clock::now();
    double     best  = 1e9;
    for (std::uint64_t epoch = 0; app.isRunning(); ++epoch) {
        double loss = 0.0;
        int    hits = 0;
        int const n = 256;
        for (int i = 0; i < n; ++i) {
            float const x = u(rng), y = u(rng), t = label(x, y);
            loss += mlp.train(x, y, t, 0.05f);
            hits += (mlp.a.back()[0] > 0.0f) == (t > 0.0f);
        }
        loss /= n;
        best = std::min(best, loss);
        app.store().push("loss", double(epoch), loss);
        app.store().push("acc", double(epoch), double(hits) / n);
        double const wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        sml::pushStats(app.store(), {epoch, best, double(epoch * n) * 0.01, wall, {}});

        float const t = float(epoch) * 0.05f; // probe input circling the ring
        mlp.forward(0.55f * std::cos(t), 0.55f * std::sin(t) * (epoch % 200 < 100 ? 1.0f : 1.6f));
        net.setGraph(mlp.graph());
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    app.join();
}
