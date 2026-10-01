#include <doctest/doctest.h>

#include <vector>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/rounded_rect.hpp"

namespace
{

bool allInside(sf::VertexArray const& va, sf::FloatRect r, float eps = 1e-3f)
{
    for (std::size_t i = 0; i < va.getVertexCount(); ++i) {
        auto const p = va[i].position;
        if (p.x < r.position.x - eps || p.y < r.position.y - eps || p.x > r.position.x + r.size.x + eps ||
            p.y > r.position.y + r.size.y + eps) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST_CASE("rounded shapes stay inside their rect, even with an oversized radius")
{
    sf::FloatRect const r{{10, 20}, {100, 40}};
    for (float radius : {0.0f, 8.0f, 500.0f}) {
        sf::VertexArray fill{sf::PrimitiveType::Triangles};
        sml::geo::roundedRect(fill, r, radius, sf::Color::White);
        CHECK(fill.getVertexCount() % 3 == 0);
        CHECK(fill.getVertexCount() > 0);
        CHECK(allInside(fill, r));

        sf::VertexArray ring{sf::PrimitiveType::Triangles};
        sml::geo::roundedRing(ring, r, radius, 5, sf::Color::White);
        CHECK(ring.getVertexCount() > 0);
        CHECK(allInside(ring, r));
    }
    sf::VertexArray empty{sf::PrimitiveType::Triangles};
    sml::geo::roundedRect(empty, {{0, 0}, {0, 10}}, 5, sf::Color::White);
    CHECK(empty.getVertexCount() == 0);
}

TEST_CASE("polyline: degenerate input, vertical segments, repeated points")
{
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    sml::geo::polyline(va, nullptr, 0, 2, sf::Color::White);
    sf::Vector2f one{1, 1};
    sml::geo::polyline(va, &one, 1, 2, sf::Color::White);
    CHECK(va.getVertexCount() == 0);

    std::vector<sf::Vector2f> pts{{0, 0}, {0, 0}, {0, 10}, {10, 10}, {10, 10}};
    sml::geo::polyline(va, pts.data(), pts.size(), 2, sf::Color::White);
    CHECK(va.getVertexCount() == 2 * 6); // two real segments
    for (std::size_t i = 0; i < va.getVertexCount(); ++i) {
        CHECK(std::isfinite(va[i].position.x));
        CHECK(std::isfinite(va[i].position.y));
    }

    // A U-turn keeps the miter bounded.
    va.clear();
    std::vector<sf::Vector2f> u{{0, 0}, {10, 0}, {0, 0.001f}};
    sml::geo::polyline(va, u.data(), u.size(), 2, sf::Color::White);
    CHECK(allInside(va, {{-3, -3}, {16, 6}}));
}

TEST_CASE("RoundedRect: setters keep color (source bug: setSize lost it)")
{
    sml::RoundedRect r{{{0, 0}, {50, 50}}, 10, sf::Color::Red};
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    r.setRect({{0, 0}, {80, 30}});
    r.appendTo(va);
    REQUIRE(va.getVertexCount() > 0);
    for (std::size_t i = 0; i < va.getVertexCount(); ++i) {
        CHECK(va[i].color == sf::Color::Red);
    }
}

TEST_CASE("shadow fades out; arrow and line skip zero length")
{
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    sml::geo::roundedShadow(va, {{0, 0}, {50, 50}}, 10, 8, {0, 0, 0, 50});
    bool sawClear = false;
    for (std::size_t i = 0; i < va.getVertexCount(); ++i) {
        sawClear = sawClear || va[i].color.a == 0;
    }
    CHECK(sawClear);
    va.clear();
    sml::geo::line(va, {1, 1}, {1, 1}, 2, sf::Color::White);
    sml::geo::arrow(va, {1, 1}, {1, 1}, 2, 4, sf::Color::White);
    CHECK(va.getVertexCount() == 0);
}
