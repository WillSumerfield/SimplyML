#include "simplyml/ui/panel.hpp"

#include <algorithm>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

Panel::Panel(std::string title, sf::Color accent)
    : m_title{std::move(title)}
    , m_accent{accent}
{}

Panel& Panel::setTitle(std::string title)
{
    if ((title.empty()) != m_title.empty()) {
        invalidate(); // content moves up/down
    }
    m_title = std::move(title);
    return *this;
}

Panel& Panel::setAccent(sf::Color accent)
{
    if (accent != m_accent) {
        m_accent = accent;
        invalidate();
    }
    return *this;
}

sf::Color Panel::accentOr(Theme const& theme) const
{
    return m_accent.a ? m_accent : theme.palette.grey;
}

void Panel::onLayout(UiContext const& ctx)
{
    Theme const&        t   = ctx.theme;
    sf::FloatRect const b   = bounds();
    float const         out = t.px(t.outline);
    sf::Vector2f const  pad = t.padding();

    m_frame.clear();
    geo::roundedShadow(m_frame, b, t.px(t.radius) + out, t.px(t.shadow), t.palette.shadow);
    geo::roundedRect(m_frame, b, t.px(t.radius) + out, t.palette.card);
    geo::roundedRing(m_frame, b, t.px(t.radius) + out, out, accentOr(t));

    m_titlePos = {b.position.x + out + 0.75f * pad.x, b.position.y + out + 0.45f * pad.y};
    float const top = m_title.empty() ? b.position.y + out + pad.x
                                      : m_titlePos.y + capHeight(ctx.font, t.pt(t.textTitle)) + t.px(t.titleGap);
    float const left   = b.position.x + out + pad.x;
    float const right  = b.position.x + b.size.x - out - pad.x;
    // Untitled panels (tiles) pad evenly; titled ones keep room at the bottom for axis labels.
    float const bottom = b.position.y + b.size.y - out - (m_title.empty() ? pad.x : pad.y);
    m_contentRect = {{left, top}, {std::max(0.0f, right - left), std::max(0.0f, bottom - top)}};

    if (m_content) {
        m_content->layout(m_contentRect, ctx);
    }
}

void Panel::update(UiContext const& ctx)
{
    if (m_content && m_content->visible()) {
        m_content->update(ctx);
    }
}

void Panel::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const& t = ctx.theme;
    target.draw(m_frame);
    if (!m_title.empty()) {
        drawText(target, ctx.font, m_title, t.pt(t.textTitle), m_titlePos, t.palette.text);
    }
    sf::FloatRect const b = bounds();
    float right = b.position.x + b.size.x - t.px(t.outline) - t.padding().x;
    // Bottom-aligned with the title so different sizes share a baseline.
    float const base = m_titlePos.y + capHeight(ctx.font, t.pt(t.textTitle));
    if (!m_valueText.empty()) {
        auto const r = drawText(target, ctx.font, m_valueText, t.pt(t.textValue), {right, base}, t.palette.text,
                                Align::End, Align::End);
        right = r.position.x - t.px(t.gap);
    }
    if (!m_legend.empty()) {
        unsigned const size = t.pt(t.textSmall);
        float const    box  = capHeight(ctx.font, size);
        sf::VertexArray keys{sf::PrimitiveType::Triangles};
        for (auto it = m_legend.rbegin(); it != m_legend.rend(); ++it) {
            auto const r = drawText(target, ctx.font, it->text, size, {right, base}, t.palette.text, Align::End,
                                    Align::End);
            float const x = r.position.x - box - t.px(6.0f);
            geo::roundedRect(keys, {{x, base - box}, {box, box}}, t.px(2.0f), it->color);
            right = x - t.px(t.gap);
        }
        target.draw(keys);
    }
    drawContent(target, ctx);
    if (m_content && m_content->visible()) {
        m_content->draw(target, ctx);
    }
}

bool Panel::handle(sf::Event const& event, UiContext const& ctx)
{
    bool consumed = Widget::handle(event, ctx);
    if (m_content) {
        consumed = m_content->handle(event, ctx) || consumed;
    }
    return consumed;
}

Widget* Panel::find(std::string_view id)
{
    if (Widget* w = Widget::find(id)) {
        return w;
    }
    return m_content ? m_content->find(id) : nullptr;
}

} // namespace sml
