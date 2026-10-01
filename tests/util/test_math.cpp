#include <doctest/doctest.h>
#include <simplyml/util/math.hpp>

namespace
{
struct V
{
    float x = 0, y = 0;
};
} // namespace

TEST_CASE("scalar helpers")
{
    CHECK(sml::sign(-3.0f) == -1.0f);
    CHECK(sml::sign(0) == 1);
    CHECK(sml::radToDeg(sml::Pi) == doctest::Approx(180.0f));
    CHECK(sml::degToRad(90.0) == doctest::Approx(sml::PiV<double> / 2));
    CHECK(sml::mix(2.0f, 4.0f, 0.25f) == doctest::Approx(2.5f));
    CHECK(sml::clampAmplitude(-5.0f, 2.0f) == -2.0f);
    CHECK(sml::clampAmplitude(1.0f, 2.0f) == 1.0f);
    CHECK(sml::sigmoid(0.0f) == doctest::Approx(0.5f));
}

TEST_CASE("vec2 helpers on any .x/.y type")
{
    V const a{3, 4}, b{1, 0};
    CHECK(sml::length(a) == doctest::Approx(5.0f));
    CHECK(sml::dot(a, b) == 3.0f);
    CHECK(sml::cross(b, V{0, 1}) == 1.0f);
    CHECK(sml::angle(b, V{0, 1}) == doctest::Approx(sml::Pi / 2));

    V const n = sml::normalize(a);
    CHECK(n.x == doctest::Approx(0.6f));
    CHECK(n.y == doctest::Approx(0.8f));

    V const z = sml::normalize(V{});
    CHECK(z.x == 0.0f);
    CHECK(z.y == 0.0f);

    V const r = sml::rotate(b, sml::Pi / 2);
    CHECK(r.x == doctest::Approx(0.0f));
    CHECK(r.y == doctest::Approx(1.0f));

    V const rd = sml::rotateDir(b, V{0, 1});
    CHECK(rd.x == doctest::Approx(0.0f));
    CHECK(rd.y == doctest::Approx(1.0f));

    V const f = sml::reflect(V{1, -1}, V{0, 1});
    CHECK(f.x == 1.0f);
    CHECK(f.y == 1.0f);

    V const p = sml::normal(b);
    CHECK(p.x == 0.0f);
    CHECK(p.y == 1.0f);
}
