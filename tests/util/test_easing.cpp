#include <doctest/doctest.h>
#include <simplyml/util/easing.hpp>

#include <cmath>

namespace
{
constexpr sml::Ease kCurves[] = {
    sml::Ease::Linear,    sml::Ease::OutCubic, sml::Ease::InOutCubic, sml::Ease::InOutQuint, sml::Ease::InOutExpo,
    sml::Ease::InOutCirc, sml::Ease::OutBack,  sml::Ease::OutElastic, sml::Ease::Sigmoid,
};
} // namespace

TEST_CASE("curves hit 0 and 1 exactly at the ends")
{
    for (auto e : kCurves) {
        CAPTURE(static_cast<int>(e));
        CHECK(sml::applyEase(e, 0.0f) == doctest::Approx(0.0f));
        CHECK(sml::applyEase(e, 1.0f) == doctest::Approx(1.0f));
    }
}

TEST_CASE("input is clamped, output finite")
{
    for (auto e : kCurves) {
        CAPTURE(static_cast<int>(e));
        CHECK(sml::applyEase(e, -3.0f) == doctest::Approx(0.0f));
        CHECK(sml::applyEase(e, 5.0f) == doctest::Approx(1.0f));
        for (int i = 0; i <= 100; ++i) {
            CHECK(std::isfinite(sml::applyEase(e, i / 100.0f)));
        }
    }
}

TEST_CASE("in-out curves are symmetric")
{
    for (auto e : {sml::Ease::InOutCubic, sml::Ease::InOutQuint, sml::Ease::InOutExpo, sml::Ease::InOutCirc,
                   sml::Ease::Sigmoid}) {
        CAPTURE(static_cast<int>(e));
        CHECK(sml::applyEase(e, 0.5f) == doctest::Approx(0.5f));
        CHECK(sml::applyEase(e, 0.2f) == doctest::Approx(1.0f - sml::applyEase(e, 0.8f)).epsilon(1e-4));
    }
}

TEST_CASE("None jumps to the end")
{
    CHECK(sml::applyEase(sml::Ease::None, 0.0f) == 1.0f);
}
