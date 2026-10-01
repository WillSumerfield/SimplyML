#include "simplyml/core/keys.hpp"

#include <array>

namespace sml
{

namespace
{

// Indexed by sf::Keyboard::Key; must follow the enum order.
constexpr std::array<std::string_view, sf::Keyboard::KeyCount> keyNames = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "escape", "lcontrol", "lshift", "lalt", "lsystem", "rcontrol", "rshift", "ralt", "rsystem", "menu",
    "lbracket", "rbracket", "semicolon", "comma", "period", "apostrophe", "slash", "backslash",
    "grave", "equal", "hyphen", "space", "enter", "backspace", "tab",
    "pageup", "pagedown", "end", "home", "insert", "delete",
    "add", "subtract", "multiply", "divide", "left", "right", "up", "down",
    "numpad0", "numpad1", "numpad2", "numpad3", "numpad4",
    "numpad5", "numpad6", "numpad7", "numpad8", "numpad9",
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
    "pause",
};
static_assert(keyNames.back() == "pause");

constexpr std::array<std::string_view, sf::Mouse::ButtonCount> buttonNames = {
    "left", "right", "middle", "extra1", "extra2",
};

} // namespace

std::string_view keyName(sf::Keyboard::Key key)
{
    auto const i = static_cast<int>(key);
    if (i < 0 || i >= static_cast<int>(keyNames.size())) {
        return "unknown";
    }
    return keyNames[static_cast<std::size_t>(i)];
}

std::optional<sf::Keyboard::Key> keyFromName(std::string_view name)
{
    for (std::size_t i = 0; i < keyNames.size(); ++i) {
        if (keyNames[i] == name) {
            return static_cast<sf::Keyboard::Key>(i);
        }
    }
    return std::nullopt;
}

std::string_view buttonName(sf::Mouse::Button button)
{
    auto const i = static_cast<std::size_t>(button);
    return i < buttonNames.size() ? buttonNames[i] : "unknown";
}

std::optional<sf::Mouse::Button> buttonFromName(std::string_view name)
{
    for (std::size_t i = 0; i < buttonNames.size(); ++i) {
        if (buttonNames[i] == name) {
            return static_cast<sf::Mouse::Button>(i);
        }
    }
    return std::nullopt;
}

} // namespace sml
