#pragma once
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include "simplyml/ui/align.hpp"

namespace sml
{

/// Size of the glyphs' bounding box.
[[nodiscard]] sf::Vector2f textSize(sf::Font const& font, std::string const& str, unsigned size);

/// Draws `str` so its glyph box is aligned to `pos` (Start = left/top). Without a transform in
/// `states` it snaps to whole pixels. Returns the glyph box (before `states`).
sf::FloatRect drawText(sf::RenderTarget& target, sf::Font const& font, std::string const& str, unsigned size,
                       sf::Vector2f pos, sf::Color color, Align h = Align::Start, Align v = Align::Start,
                       sf::RenderStates const& states = sf::RenderStates::Default);

/// Height of capital letters at `size`, for vertical layout that doesn't jump as text changes.
[[nodiscard]] float capHeight(sf::Font const& font, unsigned size);

} // namespace sml
