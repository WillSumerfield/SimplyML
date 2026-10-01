#include <doctest/doctest.h>
#include <simplyml/core/clock.hpp>

TEST_CASE("Clock ticks frames with non-negative dt")
{
    sml::Clock c;
    CHECK(c.tick() == 0.0f);
    CHECK(c.frame() == 1);
    CHECK(c.tick() >= 0.0f);
    CHECK(c.frame() == 2);
    CHECK(c.now() >= 0.0);
}

TEST_CASE("SimClock honors pause and scale")
{
    sml::SimClock s;
    s.step(1.0);
    CHECK(s.time() == doctest::Approx(1.0));
    s.setPaused(true);
    CHECK(s.step(1.0) == 0.0);
    CHECK(s.time() == doctest::Approx(1.0));
    s.togglePause();
    s.setScale(2.0);
    s.step(0.5);
    CHECK(s.time() == doctest::Approx(2.0));
    s.reset();
    CHECK(s.time() == 0.0);
}
