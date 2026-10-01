// Draws every widget off-screen; needs a display for the GL context, skipped otherwise.
#include <doctest/doctest.h>

#include <cmath>
#include <cstdlib>

#include <SFML/Graphics/RenderTexture.hpp>

#include "helpers.hpp"
#include "simplyml/ui/bar_chart.hpp"
#include "simplyml/ui/controls.hpp"
#include "simplyml/ui/key_bindings.hpp"
#include "simplyml/ui/layout.hpp"
#include "simplyml/ui/line_chart.hpp"
#include "simplyml/ui/phase_plot.hpp"
#include "simplyml/ui/ruler.hpp"
#include "simplyml/ui/stats.hpp"
#include "simplyml/ui/tracer.hpp"

namespace
{

bool hasDisplay()
{
#if defined(__linux__)
    return std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY");
#else
    return true;
#endif
}

sml::Grid& buildAll(sml::Grid& grid)
{
    grid.add<sml::LineChart>("one", "a");
    grid.add<sml::LineChart>("multi").addSeries("a").addSeries("b").addSeries("missing");
    grid.add<sml::LineChart>("window", "a").setWindow(3).setYRange(-1, 1);
    grid.add<sml::BarChart>("bars", "b").setWindow(5);
    grid.add<sml::PhasePlot>("phase", "a", "b");
    grid.add<sml::StatTile>("tile", "a");
    auto& card = grid.add<sml::StatCard>("card");
    card.addBig("big", "a");
    card.addValue("value", "b");
    card.addGauge("gauge", "a", 0, 10);
    card.addStatus("status", "b");
    auto& controls = grid.add<sml::ControlPanel>("controls");
    controls.addButton("button", "Go");
    controls.addToggle("toggle", "On", true);
    controls.addSlider("slider", "S", 1e-5, 1e-1, 1e-3).setLog(true);
    controls.addSelect("select", "Sel", {"a", "b", "c"});
    controls.add<sml::Select>("radio", "", std::vector<std::string>{"x", "y"}, 1, sml::Select::Style::Radio);
    controls.addNumber("number", "N", 3.5);
    controls.add<sml::Select>("empty", "", std::vector<std::string>{});
    grid.add<sml::KeyBindings>();
    return grid;
}

void frame(sml::Widget& w, sf::RenderTarget& target, TestUi& t, sf::FloatRect r)
{
    w.layout(r, t.ctx());
    w.update(t.ctx());
    w.draw(target, t.ctx());
    t.now += 0.1;
}

} // namespace

TEST_CASE("every widget draws with no data, one point, flat data, NaN and lots of data")
{
    if (!hasDisplay()) {
        MESSAGE("no display; skipped");
        return;
    }
    TestUi            t;
    sf::RenderTexture rt{{800, 600}};
    sml::Grid         grid{3};
    buildAll(grid);
    sf::FloatRect const full{{0, 0}, {800, 600}};

    frame(grid, rt, t, full); // no data at all

    t.store.push("a", 1.0);
    t.store.push("b", 2.0);
    t.store.sync();
    frame(grid, rt, t, full); // one point each

    for (int i = 0; i < 5; ++i) {
        t.store.push("a", 1.0); // flat
        t.store.push("b", std::nan(""));
    }
    t.store.sync();
    frame(grid, rt, t, full);

    for (int i = 0; i < 20000; ++i) {
        t.store.push("a", std::sin(i * 0.01));
        t.store.push("b", i % 7 - 3.0);
    }
    t.store.sync();
    frame(grid, rt, t, full);
    frame(grid, rt, t, {{0, 0}, {120, 90}}); // tiny
    frame(grid, rt, t, {{0, 0}, {0, 0}});    // collapsed
    rt.display();
    CHECK(grid.children().size() == 9);
}

TEST_CASE("Ruler and Tracer draw")
{
    if (!hasDisplay()) {
        MESSAGE("no display; skipped");
        return;
    }
    TestUi            t;
    sf::RenderTexture rt{{200, 100}};
    sml::Ruler        ruler{-300, 300, 10, 10, t.resources.defaultFont()};
    CHECK(ruler.tickCount() == 61);
    rt.draw(ruler);

    sml::Tracer tracer{3};
    for (int i = 0; i < 5; ++i) {
        tracer.add({float(i), float(i * i)}, i * 0.1);
    }
    CHECK(tracer.size() == 3);
    tracer.update(1.0);
    rt.draw(tracer);
}
