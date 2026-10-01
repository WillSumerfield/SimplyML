#pragma once
#include <cstddef>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>

namespace sml
{

struct Palette
{
    sf::Color accent  = {231, 111, 81};
    sf::Color orange  = {244, 162, 97};
    sf::Color yellow  = {233, 196, 106};
    sf::Color green   = {138, 177, 125};
    sf::Color teal    = {42, 157, 143};
    sf::Color blue    = {100, 170, 255};
    sf::Color grey    = {150, 150, 150};
    sf::Color card    = {50, 50, 50};    // panel body
    sf::Color clear   = {80, 80, 80};    // window background
    sf::Color text    = sf::Color::White;
    sf::Color textDim = {150, 150, 150}; // labels, ticks
    sf::Color shadow  = {0, 0, 0, 50};
    sf::Color nnPositive = {188, 226, 158};
    sf::Color nnNegative = {255, 135, 135};

    /// Cycle for multi-series charts.
    [[nodiscard]] sf::Color series(std::size_t i) const
    {
        sf::Color const cycle[] = {blue, accent, yellow, green, orange, teal};
        return cycle[i % (sizeof(cycle) / sizeof(cycle[0]))];
    }
};

/// Look of every widget: colors, sizes and the font. Sizes scale with `scale`, so the same theme
/// works at any resolution (1.0 matches the original 2560x1440 dashboard).
struct Theme
{
    Palette         palette;
    sf::Font const* font  = nullptr; // set by Ui from the App's resources if left null
    float           scale = 1.0f;

    float radius   = 20.0f; // panel corner radius
    float outline  = 5.0f;  // panel outline thickness
    float shadow   = 8.0f;
    float gap      = 20.0f; // between panels
    float margin   = 20.0f; // around the root
    float titleGap = 10.0f; // between a panel title and its content
    float tickLine = 1.0f;
    float lineWidth = 2.0f; // chart line half-width

    unsigned textSmall = 16; // labels
    unsigned textTick  = 18; // axis ticks
    unsigned textValue = 20; // readouts
    unsigned textTitle = 24; // panel titles
    unsigned textBig   = 48; // stat values

    /// Padding inside a panel's body: {radius, 1.5 * radius}.
    [[nodiscard]] sf::Vector2f padding() const { return {px(radius), px(1.5f * radius)}; }
    [[nodiscard]] float        px(float v) const { return v * scale; }
    [[nodiscard]] unsigned     pt(unsigned size) const
    {
        float const s = static_cast<float>(size) * scale;
        return s < 1.0f ? 1u : static_cast<unsigned>(s + 0.5f);
    }
};

} // namespace sml
