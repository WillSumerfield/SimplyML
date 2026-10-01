#include "simplyml/ui/controls.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

namespace
{

sf::Color mixColor(sf::Color a, sf::Color b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    auto m = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(x + (static_cast<float>(y) - x) * t));
    };
    return {m(a.r, b.r), m(a.g, b.g), m(a.b, b.b), m(a.a, b.a)};
}

sf::Color withAlpha(sf::Color c, std::uint8_t a)
{
    c.a = a;
    return c;
}

/// Small dim label over a control; returns the y below it (unchanged when there is no label).
float drawLabel(sf::RenderTarget& target, UiContext const& ctx, std::string const& label, sf::Vector2f pos, sf::Color color)
{
    if (label.empty()) {
        return pos.y;
    }
    Theme const& t = ctx.theme;
    drawText(target, ctx.font, label, t.pt(t.textSmall), pos, color);
    return pos.y + capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f);
}

float labelHeight(UiContext const& ctx, std::string const& label)
{
    Theme const& t = ctx.theme;
    return label.empty() ? 0.0f : capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f);
}

} // namespace

// Control -------------------------------------------------------------------------------------------

Control::Control(std::string name, std::string label)
    : m_label{std::move(label)}
{
    setId(std::move(name));
}

Control& Control::setValue(double value)
{
    m_value   = sanitize(value);
    m_publish = true;
    return *this;
}

void Control::update(UiContext const& ctx)
{
    if (!ctx.controls || id().empty()) {
        return;
    }
    if (m_publish) {
        m_publish = false;
        m_seen    = ctx.controls->set(id(), m_value);
        return;
    }
    auto const e = ctx.controls->entry(id());
    if (!e) {
        m_seen = ctx.controls->set(id(), m_value);
    } else if (e->version != m_seen) {
        m_value = sanitize(e->value);
        m_seen  = m_value == e->value ? e->version : ctx.controls->set(id(), m_value);
    }
}

void Control::commit(double value, UiContext const& ctx)
{
    value = sanitize(value);
    if (value == m_value) {
        return;
    }
    m_value   = value;
    m_publish = false;
    if (ctx.controls && !id().empty()) {
        m_seen = ctx.controls->set(id(), value);
    }
    if (ctx.events && !id().empty()) {
        AppEvent e;
        e.name  = id();
        e.value = value;
        ctx.events->post(std::move(e));
    }
    if (m_onChange) {
        m_onChange(value);
    }
}

sf::Color Control::fade(sf::Color c) const
{
    if (!m_enabled) {
        c.a = static_cast<std::uint8_t>(c.a * 0.4f);
    }
    return c;
}

// Button --------------------------------------------------------------------------------------------

Button::Button(std::string name, std::string label, sf::Color color)
    : Control{std::move(name), std::move(label)}
    , m_color{color}
{
    if (this->label().empty()) {
        setLabel(id());
    }
}

void Button::onClick(UiContext const& ctx)
{
    commit(value() + 1.0, ctx);
}

void Button::trigger(UiContext const& ctx)
{
    if (enabled()) {
        m_flash.setInstant(1.0f);
        m_flash.set(0.0f, ctx.now);
        commit(value() + 1.0, ctx);
    }
}

sf::Vector2f Button::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {textSize(ctx.font, label(), t.pt(t.textValue)).x + 2.0f * t.px(24.0f), t.px(44.0f)};
}

void Button::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t      = ctx.theme;
    sf::FloatRect const b      = bounds();
    sf::Color const     accent = m_color.a ? m_color : t.palette.grey;
    float const         press  = std::max(pressAmount(ctx.now), m_flash.get(ctx.now));

    sf::Color body = m_color.a ? mixColor(t.palette.card, m_color, 0.45f) : t.palette.control;
    body = mixColor(body, sf::Color::White, 0.12f * hoverAmount(ctx.now));
    body = mixColor(body, m_color.a ? m_color : t.palette.grey, 0.4f * press);

    float const     radius = std::min(t.px(10.0f), 0.5f * b.size.y);
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::roundedRect(va, b, radius, fade(body));
    geo::roundedRing(va, b, radius, t.px(2.0f), fade(accent));
    if (focused()) {
        geo::roundedRing(va, b, radius, t.px(1.0f), fade(withAlpha(t.palette.text, 120)));
    }
    target.draw(va);
    drawText(target, ctx.font, label(), t.pt(t.textValue), {b.position.x + 0.5f * b.size.x, b.position.y + 0.5f * b.size.y},
             fade(t.palette.text), Align::Center, Align::Center);
}

// Toggle --------------------------------------------------------------------------------------------

Toggle::Toggle(std::string name, std::string label, bool on)
    : Control{std::move(name), std::move(label)}
{
    initValue(on ? 1.0 : 0.0);
    m_knob.setInstant(on ? 1.0f : 0.0f);
}

void Toggle::onClick(UiContext const& ctx)
{
    commit(isOn() ? 0.0 : 1.0, ctx);
}

void Toggle::trigger(UiContext const& ctx)
{
    if (enabled()) {
        commit(isOn() ? 0.0 : 1.0, ctx);
    }
}

void Toggle::update(UiContext const& ctx)
{
    Control::update(ctx);
    m_knob.set(isOn() ? 1.0f : 0.0f, ctx.now);
}

sf::Vector2f Toggle::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {textSize(ctx.font, label(), t.pt(t.textValue)).x + t.px(16.0f) + t.px(56.0f),
            std::max(t.px(30.0f), capHeight(ctx.font, t.pt(t.textValue)))};
}

void Toggle::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t   = ctx.theme;
    sf::FloatRect const b   = bounds();
    float const         mid = b.position.y + 0.5f * b.size.y;
    drawText(target, ctx.font, label(), t.pt(t.textValue), {b.position.x, mid}, fade(t.palette.text), Align::Start,
             Align::Center);

    float const w = t.px(56.0f);
    float const h = t.px(30.0f);
    sf::FloatRect const sw{{b.position.x + b.size.x - w, mid - 0.5f * h}, {w, h}};
    float const     k     = std::clamp(m_knob.get(ctx.now), 0.0f, 1.0f);
    sf::Color const on    = m_on.a ? m_on : t.palette.green;
    sf::Color const off   = m_off.a ? m_off : t.palette.accent;
    sf::Color       track = mixColor(off, on, k);
    track = mixColor(track, sf::Color::White, 0.12f * hoverAmount(ctx.now));

    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::roundedRect(va, sw, 0.5f * h, fade(track));
    float const r = 0.5f * h - t.px(4.0f) - t.px(1.5f) * pressAmount(ctx.now);
    geo::circle(va, {sw.position.x + 0.5f * h + k * (w - h), mid}, r, fade(t.palette.text), 24);
    if (focused()) {
        geo::roundedRing(va, {sw.position - sf::Vector2f{3, 3} * t.scale, sw.size + sf::Vector2f{6, 6} * t.scale},
                         0.5f * h + t.px(3.0f), t.px(1.0f), fade(withAlpha(t.palette.text, 120)));
    }
    target.draw(va);
}

// Slider --------------------------------------------------------------------------------------------

Slider::Slider(std::string name, std::string label, double lo, double hi, double value)
    : Control{std::move(name), std::move(label)}
    , m_lo{lo}
    , m_hi{hi}
{
    initValue(value);
}

Slider& Slider::setRange(double lo, double hi)
{
    m_lo = lo;
    m_hi = hi;
    return *this;
}

double Slider::ratioOf(double v) const
{
    double r = 0.0;
    if (isLog()) {
        r = v > 0.0 ? std::log(v / m_lo) / std::log(m_hi / m_lo) : 0.0;
    } else if (m_hi != m_lo) {
        r = (v - m_lo) / (m_hi - m_lo);
    }
    return std::isfinite(r) ? std::clamp(r, 0.0, 1.0) : 0.0;
}

double Slider::valueAt(double ratio) const
{
    ratio = std::clamp(ratio, 0.0, 1.0);
    if (isLog()) {
        return m_lo * std::pow(m_hi / m_lo, ratio);
    }
    return m_lo + (m_hi - m_lo) * ratio;
}

double Slider::sanitize(double v) const
{
    if (!std::isfinite(v)) {
        return value();
    }
    if (m_step > 0.0) {
        v = m_lo + std::round((v - m_lo) / m_step) * m_step;
    }
    return std::clamp(v, std::min(m_lo, m_hi), std::max(m_lo, m_hi));
}

std::string Slider::text(double v) const
{
    if (m_format) {
        return (*m_format)(v);
    }
    if (isLog()) {
        return ValueFormat::general(3)(v);
    }
    double const step = m_step > 0.0 ? m_step : std::abs(m_hi - m_lo) / 100.0;
    return ValueFormat::number(std::clamp(decimalsFor(step), 0, 4))(v);
}

sf::FloatRect Slider::track(UiContext const& ctx) const
{
    Theme const&        t = ctx.theme;
    sf::FloatRect const b = bounds();
    float const         r = t.px(10.0f);
    float const         mid = b.position.y + (capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f)) + t.px(12.0f);
    return {{b.position.x + r, mid}, {std::max(0.0f, b.size.x - 2.0f * r), 0.0f}};
}

void Slider::dragTo(float x, UiContext const& ctx)
{
    sf::FloatRect const tr = track(ctx);
    commit(valueAt(tr.size.x > 0.0f ? (x - tr.position.x) / tr.size.x : 0.0), ctx);
}

void Slider::nudge(int steps, UiContext const& ctx)
{
    if (m_step > 0.0) {
        commit(value() + steps * m_step, ctx);
    } else {
        commit(valueAt(ratioOf(value()) + 0.01 * steps), ctx);
    }
}

bool Slider::handle(sf::Event const& event, UiContext const& ctx)
{
    bool consumed = Widget::handle(event, ctx);
    if (!enabled()) {
        return consumed;
    }
    if (auto const* p = event.getIf<sf::Event::MouseButtonPressed>(); p && pressed()) {
        dragTo(static_cast<float>(p->position.x), ctx);
    } else if (auto const* m = event.getIf<sf::Event::MouseMoved>(); m && pressed()) {
        dragTo(static_cast<float>(m->position.x), ctx);
        consumed = true;
    } else if (auto const* w = event.getIf<sf::Event::MouseWheelScrolled>(); w && hovered()) {
        nudge(w->delta > 0 ? 1 : -1, ctx);
        consumed = true;
    } else if (auto const* k = event.getIf<sf::Event::KeyPressed>(); k && focused()) {
        using K = sf::Keyboard::Key;
        switch (k->code) {
            case K::Left: case K::Down:  nudge(-1, ctx); return true;
            case K::Right: case K::Up:   nudge(1, ctx); return true;
            case K::PageDown:            nudge(-10, ctx); return true;
            case K::PageUp:              nudge(10, ctx); return true;
            case K::Home:                commit(m_lo, ctx); return true;
            case K::End:                 commit(m_hi, ctx); return true;
            default:                     break;
        }
    }
    return consumed;
}

sf::Vector2f Slider::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {t.px(160.0f), (capHeight(ctx.font, t.pt(t.textSmall)) + t.px(12.0f)) + t.px(24.0f)};
}

void Slider::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t     = ctx.theme;
    sf::FloatRect const b     = bounds();
    sf::Color const     color = m_color.a ? m_color : t.palette.blue;
    drawText(target, ctx.font, label(), t.pt(t.textSmall), b.position, fade(t.palette.textDim));
    drawText(target, ctx.font, text(value()), t.pt(t.textSmall), {b.position.x + b.size.x, b.position.y},
             fade(t.palette.text), Align::End);

    sf::FloatRect const tr  = track(ctx);
    float const         mid = tr.position.y;
    float const         bar = t.px(3.0f);
    float const         kx  = tr.position.x + static_cast<float>(ratioOf(value())) * tr.size.x;

    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::roundedRect(va, {{b.position.x, mid - bar}, {b.size.x, 2.0f * bar}}, bar, fade(t.palette.well));
    geo::roundedRect(va, {{b.position.x, mid - bar}, {kx - b.position.x, 2.0f * bar}}, bar, fade(color));
    float const r = t.px(10.0f) + t.px(2.0f) * hoverAmount(ctx.now);
    if (focused()) {
        geo::circle(va, {kx, mid}, r + t.px(5.0f), fade(withAlpha(color, 70)), 32);
    }
    geo::circle(va, {kx, mid}, r, fade(t.palette.text), 32);
    geo::circle(va, {kx, mid}, r * (0.45f + 0.15f * pressAmount(ctx.now)), fade(color), 24);
    target.draw(va);
}

// Select --------------------------------------------------------------------------------------------

Select::Select(std::string name, std::string label, std::vector<std::string> options, int index, Style style)
    : Control{std::move(name), std::move(label)}
    , m_options{std::move(options)}
    , m_style{style}
{
    initValue(index);
    m_highlight.setInstant(static_cast<float>(this->index()));
}

std::string const& Select::selected() const
{
    static std::string const none;
    return m_options.empty() ? none : m_options[static_cast<std::size_t>(index())];
}

double Select::sanitize(double v) const
{
    if (m_options.empty() || !std::isfinite(v)) {
        return 0.0;
    }
    return std::clamp(std::round(v), 0.0, static_cast<double>(m_options.size() - 1));
}

void Select::trigger(UiContext const& ctx)
{
    if (enabled() && !m_options.empty()) {
        commit((index() + 1) % static_cast<int>(m_options.size()), ctx);
    }
}

float Select::labelHeight(UiContext const& ctx) const
{
    return sml::labelHeight(ctx, label());
}

sf::FloatRect Select::optionRect(int i, UiContext const& ctx) const
{
    Theme const&        t   = ctx.theme;
    sf::FloatRect const b   = bounds();
    float const         top = b.position.y + labelHeight(ctx);
    float const         n   = static_cast<float>(std::max<std::size_t>(m_options.size(), 1));
    if (m_style == Style::Segmented) {
        float const inset = t.px(4.0f);
        float const w     = (b.size.x - 2.0f * inset) / n;
        return {{b.position.x + inset + w * static_cast<float>(i), top + inset}, {w, t.px(40.0f) - 2.0f * inset}};
    }
    float const row = t.px(30.0f);
    return {{b.position.x, top + static_cast<float>(i) * (row + t.px(8.0f))}, {b.size.x, row}};
}

int Select::optionAt(sf::Vector2f p, UiContext const& ctx) const
{
    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        if (optionRect(i, ctx).contains(p)) {
            return i;
        }
    }
    return -1;
}

void Select::onClick(UiContext const& ctx)
{
    if (int const i = optionAt(m_pressPos, ctx); i >= 0) {
        commit(i, ctx);
    }
}

bool Select::handle(sf::Event const& event, UiContext const& ctx)
{
    if (auto const* p = event.getIf<sf::Event::MouseButtonPressed>()) {
        m_pressPos = sf::Vector2f(p->position);
    }
    bool const consumed = Widget::handle(event, ctx);
    if (auto const* k = event.getIf<sf::Event::KeyPressed>(); k && focused() && enabled()) {
        using K = sf::Keyboard::Key;
        int const n = static_cast<int>(m_options.size());
        if (k->code == K::Left || k->code == K::Up) {
            commit(std::max(index() - 1, 0), ctx);
            return true;
        }
        if (k->code == K::Right || k->code == K::Down) {
            commit(std::min(index() + 1, n - 1), ctx);
            return true;
        }
    }
    return consumed;
}

void Select::update(UiContext const& ctx)
{
    Control::update(ctx);
    m_highlight.set(static_cast<float>(index()), ctx.now);
}

sf::Vector2f Select::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    float        w = 0.0f;
    for (auto const& o : m_options) {
        w = std::max(w, textSize(ctx.font, o, t.pt(t.textValue)).x);
    }
    if (m_style == Style::Segmented) {
        return {(w + t.px(24.0f)) * static_cast<float>(m_options.size()), labelHeight(ctx) + t.px(40.0f)};
    }
    float const n = static_cast<float>(m_options.size());
    return {w + t.px(44.0f), labelHeight(ctx) + n * t.px(30.0f) + std::max(0.0f, n - 1.0f) * t.px(8.0f)};
}

void Select::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t     = ctx.theme;
    sf::FloatRect const b     = bounds();
    sf::Color const     color = m_color.a ? m_color : t.palette.accent;
    unsigned const      size  = t.pt(t.textValue);
    float const         hl    = m_highlight.get(ctx.now);
    int const           hover = hovered() && enabled() ? optionAt(ctx.mouse, ctx) : -1;
    float const         top   = drawLabel(target, ctx, label(), b.position, fade(t.palette.textDim));

    sf::VertexArray va{sf::PrimitiveType::Triangles};
    if (m_style == Style::Segmented) {
        sf::FloatRect const box{{b.position.x, top}, {b.size.x, t.px(40.0f)}};
        geo::roundedRect(va, box, t.px(10.0f), fade(t.palette.well));
        if (hover >= 0 && hover != index()) {
            geo::roundedRect(va, optionRect(hover, ctx), t.px(7.0f), fade(withAlpha(t.palette.control, 160)));
        }
        if (!m_options.empty()) {
            sf::FloatRect r = optionRect(0, ctx);
            r.position.x += hl * r.size.x;
            geo::roundedRect(va, r, t.px(7.0f), fade(color));
        }
        if (focused()) {
            geo::roundedRing(va, box, t.px(10.0f), t.px(1.0f), fade(withAlpha(t.palette.text, 120)));
        }
        target.draw(va);
        for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
            sf::FloatRect const r   = optionRect(i, ctx);
            float const         sel = std::max(0.0f, 1.0f - std::abs(hl - static_cast<float>(i)));
            drawText(target, ctx.font, m_options[static_cast<std::size_t>(i)], size,
                     {r.position.x + 0.5f * r.size.x, r.position.y + 0.5f * r.size.y},
                     fade(mixColor(t.palette.textDim, t.palette.text, sel)), Align::Center, Align::Center);
        }
        return;
    }

    float const ring = t.px(11.0f);
    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        sf::FloatRect const r   = optionRect(i, ctx);
        sf::Vector2f const  c{r.position.x + ring, r.position.y + 0.5f * r.size.y};
        float const         sel = std::max(0.0f, 1.0f - std::abs(hl - static_cast<float>(i)));
        geo::circle(va, c, ring, fade(t.palette.well), 28);
        geo::ring(va, c, ring, t.px(2.0f), fade(i == hover ? t.palette.text : t.palette.grey), 28);
        if (sel > 0.01f) {
            geo::circle(va, c, ring * 0.5f * sel, fade(color), 24);
        }
    }
    target.draw(va);
    for (int i = 0; i < static_cast<int>(m_options.size()); ++i) {
        sf::FloatRect const r = optionRect(i, ctx);
        drawText(target, ctx.font, m_options[static_cast<std::size_t>(i)], size,
                 {r.position.x + 2.0f * ring + t.px(12.0f), r.position.y + 0.5f * r.size.y},
                 fade(i == index() ? t.palette.text : t.palette.textDim), Align::Start, Align::Center);
    }
}

// NumberField ---------------------------------------------------------------------------------------

NumberField::NumberField(std::string name, std::string label, double value, double lo, double hi)
    : Control{std::move(name), std::move(label)}
    , m_lo{lo}
    , m_hi{hi}
{
    initValue(value);
}

double NumberField::sanitize(double v) const
{
    if (!std::isfinite(v)) {
        return value();
    }
    if (m_integer) {
        v = std::round(v);
    }
    return std::clamp(v, m_lo, m_hi);
}

std::optional<double> NumberField::parse(std::string const& text) const
{
    if (text.empty()) {
        return std::nullopt;
    }
    char*        end = nullptr;
    double const v   = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size() || !std::isfinite(v)) {
        return std::nullopt;
    }
    double const r = m_integer ? std::round(v) : v;
    if (r < m_lo || r > m_hi) {
        return std::nullopt;
    }
    return r;
}

void NumberField::apply(UiContext const& ctx)
{
    if (auto const v = parse(m_edit)) {
        commit(*v, ctx);
    } else {
        m_errorUntil = ctx.now + 0.6;
    }
}

void NumberField::onFocusChanged(bool focused, UiContext const& ctx)
{
    if (focused) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.10g", value());
        m_edit      = buf;
        m_replace   = true;
        m_cancelled = false;
    } else {
        if (!m_cancelled) {
            apply(ctx);
        }
        m_edit.clear();
    }
}

bool NumberField::handle(sf::Event const& event, UiContext const& ctx)
{
    bool const consumed = Widget::handle(event, ctx);
    if (!focused()) {
        return consumed;
    }
    if (auto const* te = event.getIf<sf::Event::TextEntered>()) {
        char32_t const c = te->unicode;
        if (c < 128 && c >= 32 && std::strchr("0123456789.-+eE", static_cast<int>(c))) {
            if (m_replace) {
                m_edit.clear();
                m_replace = false;
            }
            m_edit += static_cast<char>(c);
        }
        return true;
    }
    if (auto const* k = event.getIf<sf::Event::KeyPressed>()) {
        using K = sf::Keyboard::Key;
        switch (k->code) {
            case K::Backspace:
                if (m_replace) {
                    m_edit.clear();
                } else if (!m_edit.empty()) {
                    m_edit.pop_back();
                }
                m_replace = false;
                break;
            case K::Enter:
                setFocused(false, ctx);
                break;
            case K::Escape:
                m_cancelled = true;
                setFocused(false, ctx);
                break;
            case K::Left: case K::Right: case K::Home: case K::End:
                m_replace = false; // no caret movement; just keep the text
                break;
            default: // printable keys arrive as TextEntered right after
                break;
        }
        return true; // typing never reaches hotkeys
    }
    return consumed || event.is<sf::Event::KeyReleased>();
}

sf::Vector2f NumberField::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return {t.px(140.0f), sml::labelHeight(ctx, label()) + t.px(40.0f)};
}

void NumberField::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    Theme const&        t    = ctx.theme;
    sf::FloatRect const b    = bounds();
    unsigned const      size = t.pt(t.textValue);
    float const         top  = drawLabel(target, ctx, label(), b.position, fade(t.palette.textDim));
    sf::FloatRect const box{{b.position.x, top}, {b.size.x, t.px(40.0f)}};
    float const         mid = box.position.y + 0.5f * box.size.y;

    sf::Color outline = focused() ? t.palette.blue : mixColor(t.palette.control, t.palette.grey, hoverAmount(ctx.now));
    if (ctx.now < m_errorUntil) {
        outline = t.palette.accent;
    }
    sf::VertexArray va{sf::PrimitiveType::Triangles};
    geo::roundedRect(va, box, t.px(8.0f), fade(t.palette.well));
    geo::roundedRing(va, box, t.px(8.0f), t.px(2.0f), fade(outline));

    std::string const shown = focused() ? m_edit : m_format(value());
    float const       x     = box.position.x + t.px(12.0f);
    float const       capH  = capHeight(ctx.font, size);
    float const       textW = shown.empty() ? 0.0f : textSize(ctx.font, shown, size).x;
    if (focused() && m_replace && !shown.empty()) {
        geo::roundedRect(va, {{x - t.px(3.0f), mid - 0.85f * capH}, {textW + t.px(6.0f), 1.7f * capH}}, t.px(3.0f),
                         withAlpha(t.palette.blue, 90));
    }
    if (focused() && std::fmod(ctx.now, 1.0) < 0.6) {
        geo::rect(va, {{x + textW + t.px(3.0f), mid - 0.75f * capH}, {std::max(1.0f, t.px(2.0f)), 1.5f * capH}},
                  t.palette.text);
    }
    target.draw(va);
    if (!shown.empty()) {
        drawText(target, ctx.font, shown, size, {x, mid}, fade(t.palette.text), Align::Start, Align::Center);
    }
}

// ControlPanel --------------------------------------------------------------------------------------

ControlPanel::ControlPanel(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
    , m_rows{&setContent<Column>()}
{}

sf::Vector2f ControlPanel::naturalSize(UiContext const& ctx) const
{
    float h     = 0.0f;
    int   shown = 0;
    for (auto const& c : m_rows->children()) {
        if (c->visible()) {
            h += c->naturalSize(ctx).y;
            ++shown;
        }
    }
    return {0.0f, chromeHeight(ctx) + h + ctx.theme.px(20.0f) * static_cast<float>(std::max(shown - 1, 0))};
}

void ControlPanel::onLayout(UiContext const& ctx)
{
    m_rows->setGap(ctx.theme.px(20.0f));
    Panel::onLayout(ctx);
}

Button& ControlPanel::addButton(std::string name, std::string label)
{
    return add<Button>(std::move(name), std::move(label));
}

Toggle& ControlPanel::addToggle(std::string name, std::string label, bool on)
{
    return add<Toggle>(std::move(name), std::move(label), on);
}

Slider& ControlPanel::addSlider(std::string name, std::string label, double lo, double hi, double value)
{
    return add<Slider>(std::move(name), std::move(label), lo, hi, value);
}

Select& ControlPanel::addSelect(std::string name, std::string label, std::vector<std::string> options, int index)
{
    return add<Select>(std::move(name), std::move(label), std::move(options), index);
}

NumberField& ControlPanel::addNumber(std::string name, std::string label, double value)
{
    return add<NumberField>(std::move(name), std::move(label), value);
}

} // namespace sml
