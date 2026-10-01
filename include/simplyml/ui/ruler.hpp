#pragma once
#include <functional>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace sml
{

struct RulerStyle
{
    float       tickHeight = 10.0f; // major ticks are twice as tall
    float       lineWidth  = 2.0f;
    unsigned    textSize   = 24;
    float       textScale  = 1.0f; // e.g. 0.4 to draw large, crisp glyphs scaled down
    float       labelGap   = 6.0f;
    sf::Color   color      = sf::Color::White;
    std::function<std::string(double)> label; // default: the value with no decimals
};

/// Horizontal scale: minor ticks every `step` from `from` to `to`, a taller major tick with a
/// label every `majorEvery` ticks. Drawn in its own units, so it works in world space (under a
/// simulation, through the Camera) or on screen.
class Ruler : public sf::Drawable
{
public:
    using Style = RulerStyle;

    Ruler() = default;
    Ruler(float from, float to, float step, int majorEvery, sf::Font const& font, Style style = {});

    void setRange(float from, float to, float step, int majorEvery);
    void setStyle(Style style);
    void setFont(sf::Font const& font) { m_font = &font; }

    [[nodiscard]] int tickCount() const;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    float           m_from = 0.0f, m_to = 100.0f, m_step = 10.0f;
    int             m_major = 10;
    sf::Font const* m_font  = nullptr;
    Style           m_style;
};

} // namespace sml
