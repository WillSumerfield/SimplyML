#pragma once
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/ui/widget.hpp"

namespace sml
{

/// The dashboard card: accent outline, dark body, shadow, title at the top-left and an optional
/// readout at the top-right. Content goes in `contentRect()`, either drawn by a subclass (charts,
/// stat cards) or by one child widget set with `setContent`.
class Panel : public Widget
{
public:
    explicit Panel(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    Panel& setTitle(std::string title);
    /// Readout at the top-right, e.g. the latest value.
    Panel& setValueText(std::string text) { m_valueText = std::move(text); return *this; }
    /// Colored keys at the top-right (drawn left of the readout, if any).
    struct LegendEntry
    {
        std::string text;
        sf::Color   color;
    };
    Panel& setLegend(std::vector<LegendEntry> legend) { m_legend = std::move(legend); return *this; }
    /// Outline color; transparent (the default) uses the theme grey.
    Panel& setAccent(sf::Color accent);
    [[nodiscard]] std::string const& title() const     { return m_title; }
    [[nodiscard]] std::string const& valueText() const { return m_valueText; }
    [[nodiscard]] sf::Color          accent() const    { return m_accent; }

    template<typename T, typename... Args>
    T& setContent(Args&&... args)
    {
        auto w = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *w;
        m_content = std::move(w);
        invalidate();
        return ref;
    }
    [[nodiscard]] Widget* content() const { return m_content.get(); }

    /// Inside the outline and padding, below the title.
    [[nodiscard]] sf::FloatRect contentRect() const { return m_contentRect; }

    void    update(UiContext const& ctx) override;
    void    draw(sf::RenderTarget& target, UiContext const& ctx) override;
    bool    handle(sf::Event const& event, UiContext const& ctx) override;
    Widget* find(std::string_view id) override;

protected:
    void onLayout(UiContext const& ctx) override;
    [[nodiscard]] bool alwaysLayout() const override { return m_content != nullptr; }
    /// Subclasses draw their content here, after the frame and title.
    virtual void drawContent(sf::RenderTarget&, UiContext const&) {}

    /// Accent of the outline, or the theme's grey when unset.
    [[nodiscard]] sf::Color accentOr(Theme const& theme) const;

private:
    std::string             m_title;
    std::string             m_valueText;
    std::vector<LegendEntry> m_legend;
    sf::Color               m_accent;
    std::unique_ptr<Widget> m_content;
    sf::FloatRect           m_contentRect;
    sf::VertexArray         m_frame{sf::PrimitiveType::Triangles};
    sf::Vector2f            m_titlePos;
};

} // namespace sml
