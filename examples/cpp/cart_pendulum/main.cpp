// A double pendulum on a cart (Verlet particles + distance constraints), drawn with
// CartPendulumView. Drive the cart with the slider or let it oscillate; Push kicks the tip; ghosts
// start from slightly different angles and drift apart (chaos). The simulation steps in the App's
// update callback, on the UI thread. Keys: P push, A auto, G ghosts, Esc quits.
#include <simplyml/core/app.hpp>
#include <simplyml/ml/cart_pendulum_view.hpp>
#include <simplyml/ui/controls.hpp>
#include <simplyml/ui/key_bindings.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/phase_plot.hpp>
#include <simplyml/ui/ui.hpp>
#include <simplyml/util/math.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

constexpr float Link    = 100.0f;
constexpr float Gravity = 900.0f;
constexpr float RailY   = 225.0f;
constexpr float RailMin = 220.0f, RailMax = 720.0f;

struct Chain
{
    sf::Vector2f base{470.0f, RailY};
    sf::Vector2f p[2], prev[2];

    explicit Chain(float a1 = 0.3f, float a2 = 0.0f) // angles from straight down
    {
        p[0] = base + Link * sf::Vector2f{std::sin(a1), std::cos(a1)};
        p[1] = p[0] + Link * sf::Vector2f{std::sin(a2), std::cos(a2)};
        prev[0] = p[0];
        prev[1] = p[1];
    }

    void step(float dt, float push)
    {
        for (int i = 0; i < 2; ++i) {
            sf::Vector2f const v = (p[i] - prev[i]) * 0.9995f;
            prev[i] = p[i];
            p[i] += v + sf::Vector2f{i == 1 ? push : 0.0f, Gravity} * dt * dt;
        }
        for (int it = 0; it < 4; ++it) {
            auto fix = [](sf::Vector2f const& a, sf::Vector2f& b, float wa, float wb, sf::Vector2f* pa) {
                sf::Vector2f const d   = b - a;
                float const        len = std::max(std::hypot(d.x, d.y), 1e-6f);
                sf::Vector2f const c   = d * ((len - Link) / len / (wa + wb));
                b -= c * wb;
                if (pa) {
                    *pa += c * wa;
                }
            };
            fix(base, p[0], 0.0f, 1.0f, nullptr); // the base is driven, not pulled
            fix(p[0], p[1], 1.0f, 1.0f, &p[0]);
        }
    }

    [[nodiscard]] sml::LinkChainState state(float push = 0.0f) const { return {base, {p[0], p[1]}, push}; }
    // Angles in degrees, 0 = hanging down: base link from vertical, mid = relative to the base link.
    [[nodiscard]] float angle1() const { return sml::radToDeg(std::atan2(p[0].x - base.x, p[0].y - base.y)); }
    [[nodiscard]] float angle2() const
    {
        float const a = std::atan2(p[1].x - p[0].x, p[1].y - p[0].y) - std::atan2(p[0].x - base.x, p[0].y - base.y);
        return sml::radToDeg(std::remainder(a, 2.0f * sml::PiV<float>));
    }
};

} // namespace

int main()
{
    sml::AppConfig config;
    config.title          = "SimplyML - cart pendulum";
    config.escToQuit      = true;
    config.cameraControls = false;
    sml::App app{config};
    sml::Ui  ui{app};
    auto const& pal = ui.theme().palette;

    auto& root = ui.setRoot<sml::Row>();
    auto& left = root.add<sml::Column>();
    left.setExtent(sml::px(340));
    auto& panel = left.add<sml::ControlPanel>("Cart", pal.accent);
    panel.setExtent(sml::fit());
    panel.addSlider("target", "Cart position", -250.0, 250.0, 0.0);
    auto& autoMove = panel.addToggle("auto", "Oscillate", true);
    auto& ghosts   = panel.addToggle("ghosts", "Ghosts", true);
    auto& push     = panel.addButton("push", "Push tip");
    left.add<sml::KeyBindings>("Keys").setExtent(sml::fit());
    ui.bindKey(sf::Keyboard::Key::P, push);
    ui.bindKey(sf::Keyboard::Key::A, autoMove);
    ui.bindKey(sf::Keyboard::Key::G, ghosts);

    auto& center = root.add<sml::Column>();
    auto& view   = center.add<sml::CartPendulumView>("", pal.accent);
    view.setRail(RailMin, RailMax, RailY).setWorld({{-25.0f, -25.0f}, {990.0f, 500.0f}});
    view.setExtent(sml::fr(2.0f));
    auto& plots = center.add<sml::Row>();
    plots.add<sml::LineChart>("Angle base", "angle1", pal.green).setWindow(400).setValueFormat(sml::ValueFormat::number(1, " deg"));
    plots.add<sml::LineChart>("Angle mid", "angle2", pal.orange).setWindow(400).setValueFormat(sml::ValueFormat::number(1, " deg"));
    plots.add<sml::PhasePlot>("Angle base vs mid", "angle1", "angle2", pal.accent).setMaxPoints(400);

    Chain              main{2.5f, 2.9f};
    std::vector<Chain> others;
    for (int i = 1; i <= 30; ++i) {
        others.emplace_back(2.5f + 0.002f * float(i), 2.9f);
    }
    double t = 0.0, pushUntil = -1.0, lastPush = app.controls().get("push", 0);
    std::uint64_t frame = 0;

    app.run([&](sml::App& a, float) {
        float const dt = 1.0f / 60.0f;
        auto&       c  = a.controls();
        if (c.get("push", 0) != lastPush) {
            lastPush  = c.get("push", 0);
            pushUntil = t + 0.25;
        }
        float const target = c.get("auto", 1) > 0.5 ? float(200.0 * std::sin(t * 1.3)) : float(c.get("target", 0));
        float const force  = t < pushUntil ? 3000.0f : 0.0f;
        int const   sub    = 8;
        for (int s = 0; s < sub; ++s) {
            float const x = std::clamp(main.base.x + (470.0f + target - main.base.x) * 0.02f, RailMin, RailMax);
            main.base.x   = x;
            main.step(dt / sub, force);
            for (auto& o : others) {
                o.base.x = x;
                o.step(dt / sub, force);
            }
        }
        t += dt;
        view.setState(main.state(force > 0.0f ? 30.0f : 0.0f));
        std::vector<sml::LinkChainState> gs;
        if (c.get("ghosts", 1) > 0.5) {
            for (auto const& o : others) {
                gs.push_back(o.state());
            }
        }
        view.setGhosts(std::move(gs));
        a.store().push("angle1", double(frame), main.angle1());
        a.store().push("angle2", double(frame), main.angle2());
        ++frame;
    });
}
