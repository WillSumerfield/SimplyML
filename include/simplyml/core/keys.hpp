#pragma once
#include <optional>
#include <string_view>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace sml
{

/// Stable lowercase key names for bindings, legends and Python: the SFML enumerator lowercased
/// ("space", "lshift", "f1", "numpad3"), except Num0..Num9 which are "0".."9". Unknown -> "unknown".
[[nodiscard]] std::string_view keyName(sf::Keyboard::Key key);
/// Inverse of `keyName` (case-sensitive); nullopt for unrecognised names.
[[nodiscard]] std::optional<sf::Keyboard::Key> keyFromName(std::string_view name);

/// "left", "right", "middle", "extra1", "extra2".
[[nodiscard]] std::string_view buttonName(sf::Mouse::Button button);
[[nodiscard]] std::optional<sf::Mouse::Button> buttonFromName(std::string_view name);

} // namespace sml
