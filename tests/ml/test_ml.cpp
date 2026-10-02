#include <doctest/doctest.h>

#include <cmath>
#include <cstdlib>

#include <SFML/Graphics/RenderTexture.hpp>

#include "../ui/helpers.hpp"
#include "simplyml/ml/cart_pendulum_view.hpp"
#include "simplyml/ml/network_view.hpp"
#include "simplyml/ml/training_stats_card.hpp"

namespace
{

// 3 inputs (labelled), 2 hidden, 1 output; layers given as sparse ids like NEAT depths.
sml::LayeredGraph smallNet()
{
    sml::LayeredGraph g;
    g.nodes = {{0, 0.5f, "a"}, {0, -1.0f, "b"}, {0, 0.0f, "c"}, {4, 0.2f, ""}, {4, 0.9f, ""}, {9, -0.3f, "out"}};
    for (int i = 0; i < 3; ++i) {
        g.edges.push_back({i, 3, 0.1f * i});
        g.edges.push_back({i, 4, -0.1f * i});
    }
    g.edges.push_back({3, 5, 1.0f});
    g.edges.push_back({4, 5, -1.0f});
    return g;
}

bool hasDisplay()
{
#if defined(__linux__)
    return std::getenv("DISPLAY") || std::getenv("WAYLAND_DISPLAY");
#else
    return true;
#endif
}

} // namespace

TEST_CASE("LayeredGraph topology and layers")
{
    auto g = smallNet();
    CHECK(g.layerCount() == 3);
    auto h = g;
    h.nodes[0].value = 7.0f;
    h.edges[0].value = 7.0f;
    CHECK(g.sameTopology(h));
    h.edges[0].to = 4;
    CHECK_FALSE(g.sameTopology(h));
    h = g;
    h.nodes[0].label = "x";
    CHECK_FALSE(g.sameTopology(h));
}

TEST_CASE("NetworkView: columns per layer, layers centered, layout only on topology change")
{
    TestUi           t;
    sml::NetworkView v;
    v.setGraph(smallNet());
    v.update(t.ctx());
    auto const n = v.naturalSize(t.ctx());
    CHECK(n.x > 0);
    CHECK(n.y > 0);
    v.layout({{0, 0}, n}, t.ctx());
    auto const& p = v.nodePositions();
    REQUIRE(p.size() == 6);
    CHECK(p[0].x == doctest::Approx(p[1].x)); // same layer, same column
    CHECK(p[3].x > p[0].x);
    CHECK(p[5].x > p[3].x);
    CHECK(p[1].y > p[0].y);
    CHECK(p[4].y - p[3].y == doctest::Approx(p[1].y - p[0].y)); // same pitch
    CHECK(0.5f * (p[3].y + p[4].y) == doctest::Approx(p[1].y)); // shorter layers centered
    CHECK(p[5].y == doctest::Approx(p[1].y));
    CHECK(v.nodeRadius() == doctest::Approx(9.0f)); // natural size -> unzoomed

    auto vals = smallNet();
    vals.nodes[0].value = -0.7f;
    v.setGraph(vals);
    v.update(t.ctx());
    CHECK(v.graph().nodes[0].value == doctest::Approx(-0.7f));
    CHECK(v.naturalSize(t.ctx()).x == doctest::Approx(n.x));

    v.layout({{0, 0}, {2.0f * n.x, 2.0f * n.y}}, t.ctx()); // bigger panel zooms, capped
    CHECK(v.nodeRadius() == doctest::Approx(9.0f * 1.5f));
}

TEST_CASE("NetworkView: placed graph fits, keeps aspect and spacing, labels face outward")
{
    TestUi           t;
    sml::NetworkView v;
    sml::PlacedGraph g;
    g.nodes = {{{0, 0}, 0.5f, "in"}, {{10, 0}, -0.2f, ""}, {{10.5f, 5}, 0.1f, ""}, {{20, 10}, 0.9f, "out"}};
    g.edges = {{0, 1, 1.0f}, {1, 2, -1.0f}, {2, 2, 0.5f}, {2, 3, 0.3f}};
    v.setGraph(g);
    v.update(t.ctx());
    REQUIRE(v.isPlaced());
    CHECK(v.graph().nodes.empty());
    sf::FloatRect const r{{0, 0}, {300, 200}};
    v.layout(r, t.ctx());
    auto const& p = v.nodePositions();
    REQUIRE(p.size() == 4);
    sf::FloatRect const c = v.contentRect();
    float const         s = (p[3].x - p[0].x) / 20.0f;
    CHECK((p[3].y - p[0].y) / 10.0f == doctest::Approx(s)); // aspect kept
    CHECK(p[0].x - v.nodeRadius() > c.position.x);          // "in" label room on the left
    CHECK(p[3].x + v.nodeRadius() < c.position.x + c.size.x);
    float const closest = (p[2] - p[1]).length();
    CHECK(closest >= 2.0f * v.nodeRadius()); // shrunk so the closest pair doesn't overlap

    auto vals = g;
    vals.nodes[1].value = 0.7f;
    v.setGraph(vals);
    v.update(t.ctx());
    CHECK(v.placedGraph().nodes[1].value == doctest::Approx(0.7f));
    CHECK(v.nodePositions()[1] == p[1]); // values only: no relayout needed

    v.setGraph(smallNet()); // back to layered
    v.update(t.ctx());
    CHECK_FALSE(v.isPlaced());
    CHECK(v.placedGraph().nodes.empty());
    v.layout(r, t.ctx());
    CHECK(v.nodePositions().size() == 6);
}

TEST_CASE("CartPendulumView fits the world into the panel")
{
    TestUi                 t;
    sml::CartPendulumView v;
    v.setRail(220, 720, 225).setWorld({{-25, -25}, {990, 500}});
    v.layout({{0, 0}, {1200, 700}}, t.ctx());
    sf::Transform const& m = v.worldTransform();
    sf::Vector2f const   a = m.transformPoint({-25, -25});
    sf::Vector2f const   b = m.transformPoint({965, 475});
    CHECK(a.x >= v.contentRect().position.x - 0.01f);
    CHECK(b.x <= v.contentRect().position.x + v.contentRect().size.x + 0.01f);
    CHECK((b.x - a.x) / 990.0f == doctest::Approx((b.y - a.y) / 500.0f)); // aspect kept
}

TEST_CASE("pushStats and TrainingStatsCard series")
{
    TestUi t;
    sml::pushStats(t.store, {7, 3.5, 3600.0, 12.0, {{"gravity", 9.8}}}, "run_");
    t.store.sync();
    REQUIRE(t.store.series("run_iteration"));
    CHECK(t.store.series("run_gravity")->last().value == 9.8);
    CHECK(t.store.series("run_best_score")->last().step == 7.0);
    sml::TrainingStatsCard card{"Run", sf::Color::Transparent, "run_"};
    CHECK(card.iteration().currentText(t.ctx()) == "7");
    CHECK(card.simTime().currentText(t.ctx()) == "1 hour, 0 minutes");
    CHECK(card.rows().children().size() == 4);
    auto& g = card.addGauge("Gravity", "run_gravity", 0, 20);
    CHECK(card.rows().move(g, 1));
    CHECK(card.rows().children()[1].get() == &g);
}

TEST_CASE("ml widgets draw empty, normal, huge and degenerate data")
{
    if (!hasDisplay()) {
        MESSAGE("no display; skipped");
        return;
    }
    TestUi                 t;
    sf::RenderTexture      rt{{800, 600}};
    sml::NetworkView       net{"net"};
    net.setEdgeWidth(0.0f, 0.5f).setEdgeAlpha(100);
    sml::CartPendulumView cart;
    sml::TrainingStatsCard stats;
    auto frame = [&](sf::FloatRect r) {
        for (sml::Widget* w : {static_cast<sml::Widget*>(&net), static_cast<sml::Widget*>(&cart),
                               static_cast<sml::Widget*>(&stats)}) {
            w->layout(r, t.ctx());
            w->update(t.ctx());
            w->draw(rt, t.ctx());
        }
    };
    frame({{0, 0}, {800, 600}});

    net.setGraph(smallNet());
    cart.setState({{0, 0}, {{0, 100}, {0, 200}}, 30.0f});
    cart.setGhosts({{{10, 0}, {{10, 100}}, 0.0f}, {{0, 0}, {}, -5.0f}});
    frame({{0, 0}, {800, 600}});

    sml::LayeredGraph big;
    for (int i = 0; i < 300; ++i) {
        big.nodes.push_back({i / 100, std::sin(float(i)), {}});
    }
    for (int i = 0; i < 100; ++i) {
        for (int k = 100; k < 200; ++k) {
            big.edges.push_back({i, k, std::nanf("")});
        }
    }
    big.edges.push_back({0, 999, 1.0f}); // out of range: skipped
    net.setGraph(big);
    frame({{0, 0}, {800, 600}});
    sml::PlacedGraph placed; // coincident nodes, a self-loop, an out-of-range edge
    placed.nodes = {{{1, 1}, 0.5f, "a"}, {{1, 1}, -0.5f, "b"}, {{1, 1}, 1.0f, ""}};
    placed.edges = {{0, 0, 1.0f}, {0, 1, 2.0f}, {1, 7, 1.0f}};
    net.setGraph(placed);
    frame({{0, 0}, {800, 600}});
    net.setGraph(big);
    sf::Texture wheel{sf::Vector2u{8, 8}};
    cart.setWheelTexture(&wheel);
    frame({{0, 0}, {800, 600}});
    frame({{0, 0}, {40, 30}});
    frame({{0, 0}, {0, 0}});
    rt.display();
    CHECK(net.graph().nodes.size() == 300);
}
