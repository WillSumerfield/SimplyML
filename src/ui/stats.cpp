#include "simplyml/ui/stats.hpp"

#include <algorithm>
#include <cmath>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

// ValueWidget ---------------------------------------------------------------------------------------

std::optional<double> ValueWidget::current(UiContext const& ctx) const
{
    if (!m_series.empty()) {
        if (Series const* s = ctx.store.series(m_series); s && !s->empty()) {
            return s->last().value;
        }
    }
    return m_value;
}

std::string ValueWidget::currentText(UiContext const& ctx) const
{
    if (m_text) {
        return *m_text;
    }
    auto const v = current(ctx);
    return v ? m_format(*v) : std::string{"-"};
}

// StatValue -----------------------------------------------------------------------------------------

StatValue::StatValue(std::string label, std::string series, ValueFormat fmt, Style style)
    : m_style{style}
{
    m_label  = std::move(label);
    m_series = std::move(series);
    m_format = std::move(fmt);
}

sf::Vector2f StatValue::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    switch (m_style) {
        case Style::Stacked:
            return {0.0f, capHeight(ctx.font, t.pt(t.textSmall)) + t.px(10.0f) + capHeight(ctx.font, t.pt(t.textValue))};
        case Style::Big: {
            float h = capHeight(ctx.font, t.pt(t.textBig));
            if (!m_label.empty()) {
                h += capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f);
            }
            return {0.0f, h};
        }
        case Style::Inline:
            return {0.0f, capHeight(ctx.font, t.pt(t.textLarge))};
    }
    return {};
}

void StatValue::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t = ctx.theme;
    sf::FloatRect const b = bounds();
    std::string const   value = currentText(ctx);
    switch (m_style) {
        case Style::Stacked: {
            auto const r = drawText(target, ctx.font, m_label, t.pt(t.textSmall), b.position, t.palette.textDim);
            drawText(target, ctx.font, value, t.pt(t.textValue),
                     {b.position.x, r.position.y + capHeight(ctx.font, t.pt(t.textSmall)) + t.px(10.0f)}, t.palette.text);
            break;
        }
        case Style::Big: {
            float y = b.position.y;
            if (!m_label.empty()) {
                drawText(target, ctx.font, m_label, t.pt(t.textSmall), b.position, t.palette.textDim);
                y += capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f);
            }
            drawText(target, ctx.font, value, t.pt(t.textBig), {b.position.x, y}, t.palette.text);
            break;
        }
        case Style::Inline: {
            float const mid = b.position.y + 0.5f * b.size.y;
            drawText(target, ctx.font, m_label, t.pt(t.textTitle), {b.position.x, mid}, t.palette.text, Align::Start,
                     Align::Center);
            drawText(target, ctx.font, value, t.pt(t.textLarge), {b.position.x + b.size.x, mid}, t.palette.text,
                     Align::End, Align::Center);
            break;
        }
    }
}

// Gauge ---------------------------------------------------------------------------------------------

Gauge::Gauge(std::string label, std::string series, double lo, double hi, sf::Color color)
    : m_lo{lo}
    , m_hi{hi}
    , m_color{color}
{
    m_label  = std::move(label);
    m_series = std::move(series);
}

double Gauge::ratio(double v) const
{
    if (!(m_hi > m_lo) || !std::isfinite(v)) {
        return 0.0;
    }
    return std::clamp((v - m_lo) / (m_hi - m_lo), 0.0, 1.0);
}

sf::Vector2f Gauge::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {0.0f, capHeight(ctx.font, t.pt(t.textSmall)) + t.px(10.0f) + t.px(20.0f)};
}

void Gauge::update(UiContext const& ctx)
{
    auto const v = current(ctx);
    m_ratio.set(static_cast<float>(v ? ratio(*v) : 0.0), ctx.now);
}

void Gauge::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t = ctx.theme;
    sf::FloatRect const b = bounds();
    unsigned const      small = t.pt(t.textSmall);
    drawText(target, ctx.font, m_label, small, b.position, t.palette.textDim);
    if (m_showValue) {
        drawText(target, ctx.font, currentText(ctx), small, {b.position.x + b.size.x, b.position.y},
                 t.palette.textDim, Align::End);
    }
    float const   barTop = b.position.y + capHeight(ctx.font, small) + t.px(10.0f);
    float const   h      = std::max(0.0f, std::min(t.px(20.0f), b.position.y + b.size.y - barTop));
    sf::FloatRect const bar{{b.position.x, barTop}, {b.size.x, h}};
    float const   line = t.px(2.0f);
    float const   inset = 2.0f * line;

    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::roundedRing(va, bar, 0.5f * h, line, t.palette.text);
    float const inner = std::max(0.0f, bar.size.x - 2.0f * inset);
    float const fill  = inner * std::clamp(m_ratio.get(ctx.now), 0.0f, 1.0f);
    if (fill > 0.5f) {
        float const fh = std::max(0.0f, h - 2.0f * inset);
        geo::roundedRect(va, {{bar.position.x + inset, barTop + inset}, {fill, fh}}, 0.5f * fh,
                         m_color.a ? m_color : t.palette.blue);
    }
    target.draw(va);
}

// StatusDot -----------------------------------------------------------------------------------------

StatusDot::StatusDot(std::string label, std::string series, std::string onText, std::string offText)
    : m_onText{std::move(onText)}
    , m_offText{std::move(offText)}
{
    m_label  = std::move(label);
    m_series = std::move(series);
}

bool StatusDot::isOn(UiContext const& ctx) const
{
    auto const v = current(ctx);
    return v && *v > m_threshold;
}

sf::Vector2f StatusDot::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {0.0f, capHeight(ctx.font, t.pt(t.textSmall)) + t.px(14.0f) + std::max(t.px(16.0f), capHeight(ctx.font, t.pt(t.textTitle)))};
}

void StatusDot::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t  = ctx.theme;
    sf::FloatRect const b  = bounds();
    bool const          on = isOn(ctx);
    drawText(target, ctx.font, m_label, t.pt(t.textSmall), b.position, t.palette.textDim);

    float const rowTop = b.position.y + capHeight(ctx.font, t.pt(t.textSmall)) + t.px(14.0f);
    float const rowH   = std::max(t.px(16.0f), capHeight(ctx.font, t.pt(t.textTitle)));
    float const mid    = rowTop + 0.5f * rowH;
    float const r      = t.px(8.0f);
    sf::Color const color = on ? (m_on.a ? m_on : t.palette.green) : (m_off.a ? m_off : t.palette.accent);
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::circle(va, {b.position.x + r, mid}, r, color, 24);
    target.draw(va);
    std::string const text = m_text ? *m_text : (on ? m_onText : m_offText);
    drawText(target, ctx.font, text, t.pt(t.textTitle), {b.position.x + 2.0f * r + t.px(12.0f), mid},
             t.palette.text, Align::Start, Align::Center);
}

// StatCard ------------------------------------------------------------------------------------------

StatCard::StatCard(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
    , m_rows{&setContent<Column>()}
{}

void StatCard::onLayout(UiContext const& ctx)
{
    m_rows->setGap(ctx.theme.px(16.0f));
    Panel::onLayout(ctx);
}

StatValue& StatCard::addValue(std::string label, std::string series, ValueFormat fmt)
{
    return addRow<StatValue>(std::move(label), std::move(series), std::move(fmt), StatValue::Style::Stacked);
}

StatValue& StatCard::addBig(std::string label, std::string series, ValueFormat fmt)
{
    return addRow<StatValue>(std::move(label), std::move(series), std::move(fmt), StatValue::Style::Big);
}

Gauge& StatCard::addGauge(std::string label, std::string series, double lo, double hi, sf::Color color)
{
    return addRow<Gauge>(std::move(label), std::move(series), lo, hi, color);
}

StatusDot& StatCard::addStatus(std::string label, std::string series, std::string onText, std::string offText)
{
    return addRow<StatusDot>(std::move(label), std::move(series), std::move(onText), std::move(offText));
}

// StatTile ------------------------------------------------------------------------------------------

StatTile::StatTile(std::string label, std::string series, ValueFormat fmt, sf::Color accent)
    : Panel{{}, accent}
    , m_value{&setContent<StatValue>(std::move(label), std::move(series), std::move(fmt), StatValue::Style::Inline)}
{}

sf::Vector2f StatTile::naturalSize(UiContext const& ctx) const
{
    Theme const& t   = ctx.theme;
    float const  out = t.px(t.outline);
    return {0.0f, 2.0f * (out + t.padding().x) + capHeight(ctx.font, t.pt(t.textLarge))};
}

} // namespace sml
