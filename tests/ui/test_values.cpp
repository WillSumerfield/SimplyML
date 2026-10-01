#include <doctest/doctest.h>

#include <cmath>

#include "helpers.hpp"
#include "simplyml/ui/axes.hpp"
#include "simplyml/ui/stats.hpp"
#include "simplyml/ui/value_format.hpp"

TEST_CASE("ValueFormat kinds")
{
    CHECK(sml::ValueFormat::number(2)(3.14159) == "3.14");
    CHECK(sml::ValueFormat::integer(4)(42) == "0042");
    CHECK(sml::ValueFormat::integer(4)(-7) == "-0007");
    CHECK(sml::ValueFormat::integer(2)(12345) == "12345");
    CHECK(sml::ValueFormat::percent(1)(0.4237) == "42.4%");
    CHECK(sml::ValueFormat::duration()(3725) == "1 hour, 2 minutes");
    CHECK(sml::ValueFormat::number(2, " s")(1.5) == "1.50 s");
    CHECK(sml::ValueFormat::number(2)(std::nan("")) == "nan");
    CHECK(sml::ValueFormat::scientific(1)(0.00123) == "1.2e-03");
    CHECK(sml::ValueFormat::general(3)(0.001) == "0.001");
    CHECK(sml::ValueFormat::general(3)(1e-5) == "1e-05");
    sml::ValueFormat custom;
    custom.custom = [](double v) { return v > 0 ? "up" : "down"; };
    CHECK(custom(1.0) == "up");
}

TEST_CASE("decimalsFor tick steps")
{
    CHECK(sml::decimalsFor(5.0) == 0);
    CHECK(sml::decimalsFor(0.5) == 1);
    CHECK(sml::decimalsFor(0.25) == 2);
    CHECK(sml::decimalsFor(0.1 * 3) == 1);
    CHECK(sml::decimalsFor(0.0) == 0);
}

TEST_CASE("niceTicks: 1-2-5 steps covering the range; flat and reversed input")
{
    auto t = sml::niceTicks(0.13, 0.87, 4);
    CHECK(t.step == doctest::Approx(0.2));
    CHECK(t.lo == doctest::Approx(0.0));
    CHECK(t.hi == doctest::Approx(1.0));
    CHECK(t.values.size() == 6);

    auto flat = sml::niceTicks(5.0, 5.0, 4);
    CHECK(flat.lo < 5.0);
    CHECK(flat.hi > 5.0);

    auto zero = sml::niceTicks(0.0, 0.0, 4);
    CHECK(zero.lo < 0.0);
    CHECK(zero.hi > 0.0);

    auto rev = sml::niceTicks(10.0, 0.0, 5);
    CHECK(rev.lo == doctest::Approx(0.0));
    CHECK(rev.hi == doctest::Approx(10.0));

    auto ints = sml::niceTicks(0.0, 3.0, 10, true);
    CHECK(ints.step == 1.0);

    auto inf = sml::niceTicks(-INFINITY, 1.0, 4);
    CHECK(std::isfinite(inf.lo));
    for (double v : sml::niceTicks(-1.0, 1.0, 4).values) {
        CHECK_FALSE((std::signbit(v) && v == 0.0)); // no "-0"
    }
}

TEST_CASE("ValueWidget resolves text, then series, then set value")
{
    TestUi t;
    sml::StatValue v{"Loss", "loss", sml::ValueFormat::number(1)};
    CHECK(v.currentText(t.ctx()) == "-");
    v.setValue(2.0);
    CHECK(v.currentText(t.ctx()) == "2.0");
    t.store.push("loss", 0.25);
    t.store.sync();
    CHECK(v.currentText(t.ctx()) == "0.2"); // series wins over the set value ("%.1f" rounds to even)
    v.setText("n/a");
    CHECK(v.currentText(t.ctx()) == "n/a");
}

TEST_CASE("Gauge ratio clamps; StatusDot threshold")
{
    TestUi t;
    sml::Gauge g{"lr", {}, 0.0, 10.0};
    CHECK(g.ratio(5.0) == doctest::Approx(0.5));
    CHECK(g.ratio(-3.0) == 0.0);
    CHECK(g.ratio(99.0) == 1.0);
    CHECK(g.ratio(std::nan("")) == 0.0);
    g.setRange(1.0, 1.0);
    CHECK(g.ratio(1.0) == 0.0);

    sml::StatusDot d{"aug", "aug"};
    CHECK_FALSE(d.isOn(t.ctx()));
    t.store.push("aug", 1.0);
    t.store.sync();
    CHECK(d.isOn(t.ctx()));
    d.setThreshold(2.0);
    CHECK_FALSE(d.isOn(t.ctx()));
}
