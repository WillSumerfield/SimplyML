#include "simplyml/ui/key_bindings.hpp"

#include <algorithm>
#include <cctype>

#include "simplyml/core/keys.hpp"
#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

std::string keyLabel(sf::Keyboard::Key key)
{
    std::string s{keyName(key)};
    if (!s.empty()) {
        s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    }
    return s;
}

KeyBindings::KeyBindings(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
{}

float KeyBindings::rowHeight(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return std::max(t.px(30.0f), capHeight(ctx.font, t.pt(t.textValue)) + t.px(14.0f));
}

sf::Vector2f KeyBindings::naturalSize(UiContext const& ctx) const
{
    float const n = ctx.keys ? static_cast<float>(ctx.keys->size()) : 0.0f;
    return {0.0f, chromeHeight(ctx) + n * rowHeight(ctx) + std::max(0.0f, n - 1.0f) * ctx.theme.px(10.0f)};
}

void KeyBindings::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    if (!ctx.keys || ctx.keys->empty()) {
        return;
    }
    Theme const&        t     = ctx.theme;
    sf::FloatRect const area  = contentRect();
    unsigned const      small = t.pt(t.textSmall);
    float const         rowH  = rowHeight(ctx);
    float const         pad   = t.px(10.0f);

    float capW = 0.0f;
    for (auto const& k : *ctx.keys) {
        capW = std::max(capW, textSize(ctx.font, keyLabel(k.key), small).x + 2.0f * pad);
    }
    capW = std::max(capW, rowH);

    sf::VertexArray caps{sf::PrimitiveType::Triangles};
    float y = area.position.y;
    std::vector<std::pair<sf::Vector2f, std::size_t>> labels;
    for (std::size_t i = 0; i < ctx.keys->size() && y + rowH <= area.position.y + area.size.y + 0.5f; ++i) {
        sf::FloatRect const cap{{area.position.x, y}, {capW, rowH}};
        geo::roundedRect(caps, cap, t.px(6.0f), t.palette.control);
        geo::rect(caps, {{cap.position.x + t.px(4.0f), cap.position.y + cap.size.y - t.px(3.0f)}, {capW - t.px(8.0f), t.px(1.5f)}},
                  sf::Color{0, 0, 0, 60}); // key edge
        labels.emplace_back(sf::Vector2f{area.position.x, y}, i);
        y += rowH + pad;
    }
    target.draw(caps);
    for (auto const& [pos, i] : labels) {
        auto const& k   = (*ctx.keys)[i];
        float const mid = pos.y + 0.5f * rowH;
        drawText(target, ctx.font, keyLabel(k.key), small, {pos.x + 0.5f * capW, mid}, t.palette.text, Align::Center,
                 Align::Center);
        drawText(target, ctx.font, k.description, t.pt(t.textValue), {pos.x + capW + t.px(14.0f), mid}, t.palette.text,
                 Align::Start, Align::Center);
    }
}

} // namespace sml
