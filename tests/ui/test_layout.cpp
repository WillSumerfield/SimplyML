#include <doctest/doctest.h>

#include "helpers.hpp"
#include "simplyml/ui/stats.hpp"
#include "simplyml/ui/layout.hpp"

using sml::fr;
using sml::px;

TEST_CASE("distribute: fixed first, then fr shares, with gaps")
{
    auto s = sml::distribute({px(100), fr(1), fr(3)}, 520, 10);
    REQUIRE(s.size() == 3);
    CHECK(s[0].start == 0);
    CHECK(s[0].size == 100);
    CHECK(s[1].start == 110);
    CHECK(s[1].size == doctest::Approx(100));
    CHECK(s[2].start == doctest::Approx(220));
    CHECK(s[2].size == doctest::Approx(300));
}

TEST_CASE("distribute: alignment of leftover space without fr; overflow never negative")
{
    auto c = sml::distribute({px(100), px(100)}, 400, 0, sml::Align::Center);
    CHECK(c[0].start == 100);
    auto e = sml::distribute({px(100)}, 400, 0, sml::Align::End);
    CHECK(e[0].start == 300);
    auto o = sml::distribute({px(100), fr(1)}, 50, 10);
    CHECK(o[1].size == 0);
    CHECK(sml::distribute({}, 100, 10).empty());
}

TEST_CASE("Row places children by extent and skips hidden ones")
{
    TestUi t;
    sml::Row row;
    row.setGap(10);
    auto& a = row.add<Probe>();
    a.setExtent(px(50));
    auto& hidden = row.add<Probe>();
    hidden.setVisible(false);
    auto& b = row.add<Probe>();
    row.layout({{0, 0}, {260, 40}}, t.ctx());
    CHECK(a.bounds() == sf::FloatRect{{0, 0}, {50, 40}});
    CHECK(b.bounds() == sf::FloatRect{{60, 0}, {200, 40}});
    CHECK(hidden.layouts == 0);

    // Re-layout with the same size: children aren't re-laid out.
    row.layout({{0, 0}, {260, 40}}, t.ctx());
    CHECK(a.layouts == 1);
    row.layout({{0, 0}, {300, 40}}, t.ctx());
    CHECK(b.layouts == 2);
}

TEST_CASE("Column with padding")
{
    TestUi t;
    sml::Column col;
    col.setPadding(5).setGap(0);
    auto& a = col.add<Probe>();
    auto& b = col.add<Probe>();
    col.layout({{10, 10}, {100, 110}}, t.ctx());
    CHECK(a.bounds() == sf::FloatRect{{15, 15}, {90, 50}});
    CHECK(b.bounds() == sf::FloatRect{{15, 65}, {90, 50}});
}

TEST_CASE("Grid auto-flow with spans")
{
    TestUi t;
    sml::Grid grid{3};
    grid.setGap(0);
    auto& wide = grid.add<Probe>();
    wide.setSpan(2);
    auto& a = grid.add<Probe>();
    auto& tall = grid.add<Probe>();
    tall.setSpan(1, 2);
    auto& b = grid.add<Probe>();
    auto& c = grid.add<Probe>();
    grid.layout({{0, 0}, {300, 300}}, t.ctx());
    // row 0: wide wide a | row 1: tall b c | row 2: tall . .
    CHECK(wide.bounds() == sf::FloatRect{{0, 0}, {200, 100}});
    CHECK(a.bounds() == sf::FloatRect{{200, 0}, {100, 100}});
    CHECK(tall.bounds() == sf::FloatRect{{0, 100}, {100, 200}});
    CHECK(b.bounds() == sf::FloatRect{{100, 100}, {100, 100}});
    CHECK(c.bounds() == sf::FloatRect{{200, 100}, {100, 100}});
}

TEST_CASE("Grid auto columns reflow with width; spans clamp to the column count")
{
    TestUi t;
    sml::Grid grid;
    grid.setAutoColumns(100).setGap(10).setRowHeight(50);
    auto& wide = grid.add<Probe>();
    wide.setSpan(3);
    for (int i = 0; i < 3; ++i) {
        grid.add<Probe>();
    }
    grid.layout({{0, 0}, {430, 400}}, t.ctx()); // (430 + 10) / 110 = 4 columns
    CHECK(grid.columnCount() == 4);
    CHECK(wide.bounds().size.x == doctest::Approx(3 * 100 + 2 * 10 + 0.0f));

    grid.layout({{0, 0}, {150, 400}}, t.ctx()); // 1 column
    CHECK(grid.columnCount() == 1);
    CHECK(wide.bounds().size.x == doctest::Approx(150));
    auto const& last = *grid.children().back();
    CHECK(last.bounds().position.y == doctest::Approx(3 * 60));
}

TEST_CASE("find by id through containers")
{
    sml::Column col;
    auto& row = col.add<sml::Row>();
    auto& p = row.add<Probe>();
    p.setId("loss");
    CHECK(col.find("loss") == &p);
    CHECK(col.find("nope") == nullptr);
    CHECK(col.find("") == nullptr);
    CHECK(row.remove(p));
    CHECK(col.find("loss") == nullptr);
}

TEST_CASE("Widget: hover, click and focus; only interactive widgets consume")
{
    TestUi t;
    sml::Row row;
    auto& a = row.add<Probe>();
    a.clickable = true;
    auto& b = row.add<Probe>();
    row.setGap(0);
    row.layout({{0, 0}, {200, 100}}, t.ctx());

    auto press   = [](float x) { return sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {int(x), 50}}}; };
    auto release = [](float x) { return sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {int(x), 50}}}; };

    row.handle(sf::Event{sf::Event::MouseMoved{{50, 50}}}, t.ctx());
    CHECK(a.hovered());
    CHECK_FALSE(b.hovered());

    CHECK(row.handle(press(50), t.ctx()));
    CHECK(a.pressed());
    CHECK(a.focused());
    CHECK(row.handle(release(60), t.ctx()));
    CHECK(a.clicks == 1);

    // Press inside, release outside: no click.
    row.handle(press(50), t.ctx());
    row.handle(release(150), t.ctx());
    CHECK(a.clicks == 1);

    // Pressing the non-interactive widget isn't consumed but moves focus away.
    CHECK_FALSE(row.handle(press(150), t.ctx()));
    CHECK_FALSE(a.focused());
    CHECK_FALSE(b.focused());
}

TEST_CASE("Stack natural size: fixed px or natural along the axis, gaps, padding; max across")
{
    TestUi     t;
    sml::Column col;
    col.setGap(10).setPadding(5);
    auto& tile = col.add<sml::StatTile>("Epoch", "epoch");
    tile.setExtent(sml::fit());
    col.add<Probe>().setExtent(sml::px(40));
    col.add<Probe>().setVisible(false);
    float const tileH = tile.naturalSize(t.ctx()).y;
    auto const  n     = col.naturalSize(t.ctx());
    CHECK(n.y == doctest::Approx(tileH + 40 + 10 + 2 * 5));
    CHECK(n.x == doctest::Approx(tile.naturalSize(t.ctx()).x + 2 * 5));
}
