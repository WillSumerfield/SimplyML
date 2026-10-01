#include <doctest/doctest.h>

#include "simplyml/core/keys.hpp"

using sf::Keyboard::Key;

TEST_CASE("keyName covers every key and round-trips")
{
    CHECK(sml::keyName(Key::A) == "a");
    CHECK(sml::keyName(Key::Num7) == "7");
    CHECK(sml::keyName(Key::Space) == "space");
    CHECK(sml::keyName(Key::Numpad3) == "numpad3");
    CHECK(sml::keyName(Key::F12) == "f12");
    CHECK(sml::keyName(Key::Pause) == "pause");
    CHECK(sml::keyName(Key::Unknown) == "unknown");
    for (unsigned i = 0; i < sf::Keyboard::KeyCount; ++i) {
        auto const k = static_cast<Key>(i);
        CHECK(sml::keyFromName(sml::keyName(k)) == k);
    }
    CHECK_FALSE(sml::keyFromName("nope").has_value());
    CHECK_FALSE(sml::keyFromName("unknown").has_value());
}

TEST_CASE("buttonName round-trips")
{
    CHECK(sml::buttonName(sf::Mouse::Button::Middle) == "middle");
    for (unsigned i = 0; i < sf::Mouse::ButtonCount; ++i) {
        auto const b = static_cast<sf::Mouse::Button>(i);
        CHECK(sml::buttonFromName(sml::buttonName(b)) == b);
    }
    CHECK_FALSE(sml::buttonFromName("Left").has_value());
}
