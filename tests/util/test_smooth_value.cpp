#include <doctest/doctest.h>
#include <simplyml/util/smooth_value.hpp>

TEST_CASE("starts at its initial value, not {}")
{
    sml::SmoothFloat v{5.0f, 1.0f, sml::Ease::Linear};
    CHECK(v.get(0.0) == 5.0f);
    CHECK(v.get(100.0) == 5.0f);
    CHECK(v.done(0.0));

    v.set(7.0f, 10.0);
    CHECK(v.get(10.0) == doctest::Approx(5.0f));
    CHECK(v.get(10.5) == doctest::Approx(6.0f));
    CHECK(v.get(11.0) == 7.0f);
    CHECK(v.done(11.0));
}

TEST_CASE("retarget mid-animation continues from current value")
{
    sml::SmoothFloat v{0.0f, 1.0f, sml::Ease::Linear};
    v.set(10.0f, 0.0);
    v.set(0.0f, 0.5); // at 5 now
    CHECK(v.get(0.5) == doctest::Approx(5.0f));
    CHECK(v.get(1.0) == doctest::Approx(2.5f));
}

TEST_CASE("setting the same target does not restart")
{
    sml::SmoothFloat v{0.0f, 1.0f, sml::Ease::Linear};
    v.set(1.0f, 0.0);
    v.set(1.0f, 0.5);
    CHECK(v.get(0.75) == doctest::Approx(0.75f));
}

TEST_CASE("duration change applies to the next set, no jump")
{
    sml::SmoothFloat v{0.0f, 1.0f, sml::Ease::Linear};
    v.set(1.0f, 0.0);
    v.setDuration(10.0f);
    CHECK(v.get(0.5) == doctest::Approx(0.5f));
    v.set(2.0f, 1.0);
    CHECK(v.get(6.0) == doctest::Approx(1.5f));
}

TEST_CASE("zero duration and setInstant")
{
    sml::SmoothFloat v{0.0f, 0.0f};
    v.set(3.0f, 1.0);
    CHECK(v.get(1.0) == 3.0f);
    v.setInstant(-1.0f);
    CHECK(v.get(0.0) == -1.0f);
    CHECK(v.target() == -1.0f);
}

TEST_CASE("default constructed")
{
    sml::SmoothFloat v;
    CHECK(v.get(0.0) == 0.0f);
    v.set(1.0f, 0.0);
    CHECK(v.get(0.0) == doctest::Approx(0.0f));
    CHECK(v.get(1.0) == 1.0f);
}
