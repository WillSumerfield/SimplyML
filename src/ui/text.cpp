#include "simplyml/ui/text.hpp"

#include <cmath>

#include <SFML/Graphics/Text.hpp>

namespace sml
{

namespace
{

float alignOffset(Align a, float extent)
{
    switch (a) {
        case Align::Start:  return 0.0f;
        case Align::Center: return -0.5f * extent;
        case Align::End:    return -extent;
    }
    return 0.0f;
}

} // namespace

sf::Vector2f textSize(sf::Font const& font, std::string const& str, unsigned size)
{
    return sf::Text{font, str, size}.getLocalBounds().size;
}

float capHeight(sf::Font const& font, unsigned size)
{
    return font.getGlyph(U'H', size, false).bounds.size.y;
}

sf::FloatRect drawText(sf::RenderTarget& target, sf::Font const& font, std::string const& str, unsigned size,
                       sf::Vector2f pos, sf::Color color, Align h, Align v, sf::RenderStates const& states)
{
    sf::Text text{font, str, size};
    text.setFillColor(color);
    sf::FloatRect const local = text.getLocalBounds();
    // Vertical alignment uses the cap height, so a row of labels shares a baseline regardless of
    // descenders; horizontal uses the glyph box.
    // SFML puts the baseline at y = size; 'H' tells where capitals start above it.
    float const cap       = capHeight(font, size);
    float const ascentTop = static_cast<float>(size) - cap;
    sf::Vector2f p{pos.x + alignOffset(h, local.size.x) - local.position.x,
                   pos.y + alignOffset(v, cap) - ascentTop};
    if (states.transform == sf::Transform::Identity) {
        p = {std::round(p.x), std::round(p.y)};
    }
    text.setPosition(p);
    target.draw(text, states);
    return {{p.x + local.position.x, p.y + ascentTop}, {local.size.x, cap}};
}

} // namespace sml
