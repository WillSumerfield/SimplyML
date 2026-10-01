#include <doctest/doctest.h>
#include <simplyml/core/default_font.hpp>

#include <SFML/Graphics/Font.hpp>

TEST_CASE("embedded default font loads")
{
    auto const bytes = sml::defaultFontData();
    REQUIRE(bytes.size > 1000);
    sf::Font font;
    REQUIRE(font.openFromMemory(bytes.data, bytes.size));
    CHECK(font.getInfo().family == "Share Tech Mono");
}
