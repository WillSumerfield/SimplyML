#include "simplyml/ml/cart_pendulum_view.hpp"

#include <algorithm>
#include <cmath>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/util/math.hpp"

namespace sml
{

namespace
{

// Original scene sizes, world units.
constexpr float LinkWidth     = 4.0f;
constexpr float JointRadius   = 10.0f;
constexpr float JointOutline  = 2.0f;
constexpr float WheelRadius   = 12.0f;
constexpr float SmallWheel    = 0.75f; // scale of the wheels above the rail
constexpr float CartHalfWidth = 32.0f;
constexpr float AxleX         = 25.0f;
constexpr float AxleY         = 5.0f; // rail half height + outline
constexpr float RailHeight    = 6.0f;
constexpr float RailOutline   = 2.0f;
constexpr float RulerGap      = 40.0f; // rail to ruler axis

sf::Color withAlpha(sf::Color c, std::uint8_t a)
{
    c.a = static_cast<std::uint8_t>(c.a * a / 255);
    return c;
}

void triangle(sf::VertexArray& va, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color)
{
    va.append({a, color});
    va.append({b, color});
    va.append({c, color});
}

} // namespace

CartPendulumView::CartPendulumView(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
{}

CartPendulumView& CartPendulumView::setRail(float from, float to, float y, float overhang)
{
    m_railFrom = std::min(from, to);
    m_railTo   = std::max(from, to);
    m_railY    = y;
    m_overhang = overhang;
    invalidate();
    return *this;
}

CartPendulumView& CartPendulumView::setRuler(bool show, float halfWidth, float step, int majorEvery)
{
    m_ruler      = show;
    m_rulerHalf  = halfWidth;
    m_rulerStep  = step;
    m_rulerMajor = majorEvery;
    return *this;
}

sf::FloatRect CartPendulumView::world() const
{
    if (m_worldSet) {
        return m_world;
    }
    float const cx   = 0.5f * (m_railFrom + m_railTo);
    float const half = 0.5f * (m_railTo - m_railFrom) + 240.0f;
    return {{cx - half, m_railY - 250.0f}, {2.0f * half, 500.0f}};
}

void CartPendulumView::update(UiContext const& ctx)
{
    Panel::update(ctx);
    m_current       = m_state.version() != 0 ? &m_state.get() : nullptr;
    m_currentGhosts = &m_ghosts.get();
}

void CartPendulumView::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    sf::FloatRect const c = contentRect();
    sf::FloatRect const w = world();
    m_zoom = (w.size.x > 0.0f && w.size.y > 0.0f) ? std::min(c.size.x / w.size.x, c.size.y / w.size.y) : 1.0f;
    m_toScreen = sf::Transform::Identity;
    m_toScreen.translate({c.position.x + 0.5f * (c.size.x - w.size.x * m_zoom),
                          c.position.y + 0.5f * (c.size.y - w.size.y * m_zoom)});
    m_toScreen.scale({m_zoom, m_zoom});
    m_toScreen.translate(-w.position);
}

void CartPendulumView::drawChain(sf::VertexArray& va, LinkChainState const& s, std::uint8_t alpha, Palette const& pal) const
{
    sf::Color const white = withAlpha(pal.text, alpha);
    sf::Vector2f const p  = s.base;

    // Links, shortened so they stop at the joints' outlines (matters when faded).
    float const inset = JointRadius + JointOutline;
    sf::Vector2f prev = p;
    for (auto const& j : s.joints) {
        sf::Vector2f const d   = j - prev;
        float const        len = std::hypot(d.x, d.y);
        if (len > 2.0f * inset) {
            sf::Vector2f const u = d / len;
            geo::line(va, prev + u * inset, j - u * inset, LinkWidth, white);
        }
        prev = j;
    }

    // Cart: body, legs to the wheels, pivot dot.
    sf::Vector2f const w1{p.x + AxleX, p.y + AxleY + WheelRadius};
    sf::Vector2f const w2{p.x - AxleX, p.y + AxleY + WheelRadius};
    sf::Vector2f const w3{p.x + 1.5f * AxleX, p.y - AxleY - WheelRadius * SmallWheel};
    sf::Vector2f const w4{p.x - 1.5f * AxleX, p.y - AxleY - WheelRadius * SmallWheel};
    geo::line(va, {p.x - CartHalfWidth, p.y}, {p.x + CartHalfWidth, p.y}, 6.0f, white);
    geo::line(va, w1, w3, 8.0f, white);
    geo::line(va, w2, w4, 8.0f, white);
    geo::circle(va, p, 3.0f, white, 12);
    if (!m_wheel) {
        float const angle = s.base.x / WheelRadius;
        sf::Vector2f const centers[] = {w1, w2, w3, w4};
        for (int i = 0; i < 4; ++i) {
            float const r = WheelRadius * (i < 2 ? 1.0f : SmallWheel);
            float const a = i < 2 ? angle : -angle / SmallWheel;
            geo::ring(va, centers[i], r, 0.22f * r, white, 20);
            geo::circle(va, centers[i], 0.25f * r, white, 10);
            for (int k = 0; k < 5; ++k) {
                float const t = a + static_cast<float>(k) * 2.0f * PiV<float> / 5.0f;
                geo::line(va, centers[i], centers[i] + r * 0.85f * sf::Vector2f{std::cos(t), std::sin(t)}, 0.18f * r, white);
            }
        }
    }

    // Joints: base, then each link end.
    std::vector<sf::Color> const defaults{pal.green, pal.orange, pal.accent};
    auto const& colors = m_jointColors.empty() ? defaults : m_jointColors;
    for (std::size_t i = 0; i <= s.joints.size(); ++i) {
        sf::Vector2f const c = i == 0 ? p : s.joints[i - 1];
        geo::circle(va, c, JointRadius + JointOutline, white, 24);
        geo::circle(va, c, JointRadius, withAlpha(colors[i % colors.size()], alpha), 24);
    }
}

void CartPendulumView::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    Palette const&  pal = ctx.theme.palette;
    sf::RenderStates states;
    states.transform = m_toScreen;

    sf::VertexArray va{sf::PrimitiveType::Triangles};
    // Rail: rounded outline around the travel range plus overhang.
    sf::FloatRect const rail{{m_railFrom - m_overhang, m_railY - 0.5f * RailHeight},
                             {m_railTo - m_railFrom + 2.0f * m_overhang, RailHeight}};
    geo::roundedRing(va, rail, 0.5f * RailHeight, RailOutline, pal.text);
    target.draw(va, states);

    if (m_ruler) {
        float const origin = m_originSet ? m_rulerOrigin : 0.5f * (m_railFrom + m_railTo);
        Ruler::Style style;
        style.tickHeight = 10.0f;
        style.centered   = true;
        style.baseline   = false;
        style.lineWidth  = 1.5f / std::max(m_zoom, 0.01f); // ~1.5 px at any zoom
        style.textSize   = 60;
        style.textScale  = 0.2f;
        style.zeroScale  = 2.0f;
        style.labelGap   = 2.0f;
        style.color      = pal.text;
        style.majorColor = pal.accent;
        Ruler const ruler{-m_rulerHalf, m_rulerHalf, m_rulerStep, m_rulerMajor, ctx.font, style};
        sf::RenderStates rs = states;
        rs.transform.translate({origin, m_railY + RulerGap});
        target.draw(ruler, rs);
    }

    // Textured wheels of every chain, batched: two big below the rail, two small above.
    sf::VertexArray wheels{sf::PrimitiveType::Triangles};
    auto addWheels = [&](LinkChainState const& s, std::uint8_t alpha) {
        if (!m_wheel) {
            return;
        }
        sf::Vector2f const p     = s.base;
        float const        angle = s.base.x / WheelRadius;
        sf::Vector2f const ts    = sf::Vector2f(m_wheel->getSize());
        sf::Color const    color = withAlpha(pal.text, alpha);
        struct W { sf::Vector2f c; float r, a; };
        W const ws[] = {{{p.x + AxleX, p.y + AxleY + WheelRadius}, WheelRadius, angle},
                        {{p.x - AxleX, p.y + AxleY + WheelRadius}, WheelRadius, angle},
                        {{p.x + 1.5f * AxleX, p.y - AxleY - WheelRadius * SmallWheel}, WheelRadius * SmallWheel, -angle / SmallWheel},
                        {{p.x - 1.5f * AxleX, p.y - AxleY - WheelRadius * SmallWheel}, WheelRadius * SmallWheel, -angle / SmallWheel}};
        for (auto const& w : ws) {
            float const cs = std::cos(w.a) * w.r, sn = std::sin(w.a) * w.r;
            auto corner = [&](float x, float y) { return w.c + sf::Vector2f{x * cs - y * sn, x * sn + y * cs}; };
            sf::Vertex const q[4] = {{corner(-1, -1), color, {0, 0}}, {corner(1, -1), color, {ts.x, 0}},
                                     {corner(1, 1), color, ts}, {corner(-1, 1), color, {0, ts.y}}};
            for (int i : {0, 1, 2, 0, 2, 3}) {
                wheels.append(q[i]);
            }
        }
    };

    sf::RenderStates wheelStates = states;
    wheelStates.texture          = m_wheel;
    if (m_currentGhosts && !m_currentGhosts->empty()) {
        va.clear();
        wheels.clear();
        for (auto const& g : *m_currentGhosts) {
            drawChain(va, g, m_ghostAlpha, pal);
            addWheels(g, m_ghostAlpha);
        }
        target.draw(va, states);
        if (m_wheel) {
            target.draw(wheels, wheelStates);
        }
    }
    if (!m_current) {
        return;
    }
    va.clear();
    wheels.clear();
    drawChain(va, *m_current, 255, pal);
    addWheels(*m_current, 255);
    if (m_current->push != 0.0f && !m_current->joints.empty()) {
        // Arrow ending at the tip, pointing along the push (proportions of the original texture).
        sf::Vector2f const tip  = m_current->joints.back();
        float const        f    = std::abs(m_current->push);
        float const        dir  = m_current->push > 0.0f ? 1.0f : -1.0f;
        float const        len  = m_pushScale * f;
        float const        head = 0.45f * len;
        float const        hw   = 0.22f * len; // head half width
        float const        sw   = 0.09f * len; // shaft half width
        sf::Color const    red{255, 40, 40};
        sf::Vector2f const neck{tip.x - dir * head, tip.y};
        sf::Vector2f const tail{tip.x - dir * len, tip.y};
        triangle(va, {tail.x, tail.y - sw}, {neck.x, neck.y - sw}, {neck.x, neck.y + sw}, red);
        triangle(va, {tail.x, tail.y - sw}, {neck.x, neck.y + sw}, {tail.x, tail.y + sw}, red);
        triangle(va, {neck.x, neck.y - hw}, tip, {neck.x, neck.y + hw}, red);
    }
    target.draw(va, states);
    if (m_wheel) {
        target.draw(wheels, wheelStates);
    }
}

} // namespace sml
