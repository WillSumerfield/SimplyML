#include <doctest/doctest.h>
#include <simplyml/core/default_font.hpp>
#include <simplyml/core/resources.hpp>

TEST_CASE("Resources: default font always present")
{
    sml::Resources res;
    CHECK(res.defaultFont().getInfo().family == "Share Tech Mono");
}

TEST_CASE("Resources: failures and unknown names are reported")
{
    sml::Resources res;
    CHECK_THROWS_AS(res.loadFont("x", "does/not/exist.ttf"), sml::ResourceError);
    CHECK_THROWS_AS((void)res.font("nope"), sml::ResourceError);
    CHECK_THROWS_AS((void)res.texture("nope"), sml::ResourceError);
    CHECK(res.findFont("nope") == nullptr);
    CHECK(res.findTexture("nope") == nullptr);
}

TEST_CASE("Resources: named fonts and textures")
{
    sml::Resources res;
    auto const bytes = sml::defaultFontData();
    res.loadFontFromMemory("mono", bytes.data, bytes.size);
    CHECK(&res.font("mono") == res.findFont("mono"));
    CHECK(res.font("mono").getInfo().family == "Share Tech Mono");
    CHECK_THROWS_AS(res.loadFontFromMemory("bad", bytes.data, 16), sml::ResourceError);
    CHECK(res.findFont("bad") == nullptr);
}
