#include "simplyml/ui/geometry.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "simplyml/util/math.hpp"

namespace sml::geo
{

namespace
{

using V = sf::Vector2f;

void tri(sf::VertexArray& va, V a, sf::Color ca, V b, sf::Color cb, V c, sf::Color cc)
{
    va.append({a, ca});
    va.append({b, cb});
    va.append({c, cc});
}

void quad(sf::VertexArray& va, V a, V b, V c, V d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
{
    tri(va, a, ca, b, cb, c, cc);
    tri(va, a, ca, c, cc, d, cd);
}

float clampRadius(sf::FloatRect r, float radius)
{
    return std::clamp(radius, 0.0f, 0.5f * std::min(r.size.x, r.size.y));
}

/// Outline of a rounded rect, clockwise from the top-left corner, `offset` px outward.
std::vector<V> roundedOutline(sf::FloatRect r, float radius, float offset)
{
    radius = clampRadius(r, radius);
    unsigned const n = radius > 0.0f ? arcSegments(radius) : 1;
    V const centers[] = {
        {r.position.x + radius, r.position.y + radius},
        {r.position.x + r.size.x - radius, r.position.y + radius},
        {r.position.x + r.size.x - radius, r.position.y + r.size.y - radius},
        {r.position.x + radius, r.position.y + r.size.y - radius},
    };
    std::vector<V> out;
    out.reserve(4 * (n + 1));
    float const rr = radius + offset;
    for (int c = 0; c < 4; ++c) {
        float const start = Pi + static_cast<float>(c) * 0.5f * Pi;
        for (unsigned i = 0; i <= n; ++i) {
            float const a = start + 0.5f * Pi * static_cast<float>(i) / static_cast<float>(n);
            out.push_back(centers[c] + V{std::cos(a), std::sin(a)} * rr);
        }
    }
    return out;
}

void band(sf::VertexArray& va, std::vector<V> const& inner, std::vector<V> const& outer, sf::Color ci, sf::Color co)
{
    std::size_t const n = inner.size();
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t const j = (i + 1) % n;
        quad(va, inner[i], outer[i], outer[j], inner[j], ci, co, co, ci);
    }
}

} // namespace

unsigned arcSegments(float radius)
{
    return static_cast<unsigned>(std::clamp(radius * 0.5f, 3.0f, 24.0f));
}

void rect(sf::VertexArray& va, sf::FloatRect r, sf::Color color)
{
    V const p = r.position;
    V const s = r.size;
    quad(va, p, {p.x + s.x, p.y}, p + s, {p.x, p.y + s.y}, color, color, color, color);
}

void roundedRect(sf::VertexArray& va, sf::FloatRect r, float radius, sf::Color color)
{
    if (r.size.x <= 0.0f || r.size.y <= 0.0f) {
        return;
    }
    auto const pts = roundedOutline(r, radius, 0.0f);
    V const    c   = r.getCenter();
    for (std::size_t i = 0; i < pts.size(); ++i) {
        tri(va, c, color, pts[i], color, pts[(i + 1) % pts.size()], color);
    }
}

void roundedRing(sf::VertexArray& va, sf::FloatRect r, float radius, float thickness, sf::Color color)
{
    if (r.size.x <= 0.0f || r.size.y <= 0.0f || thickness <= 0.0f) {
        return;
    }
    thickness = std::min(thickness, 0.5f * std::min(r.size.x, r.size.y));
    // Inner rect with radius - thickness, offset outward by thickness: concentric arcs, same count.
    sf::FloatRect const in{r.position + V{thickness, thickness}, r.size - V{2 * thickness, 2 * thickness}};
    float const innerRadius = std::max(0.0f, clampRadius(r, radius) - thickness);
    auto const  inner       = roundedOutline(in, innerRadius, 0.0f);
    auto const  outer       = roundedOutline(in, innerRadius, thickness);
    band(va, inner, outer, color, color);
}

void roundedShadow(sf::VertexArray& va, sf::FloatRect r, float radius, float size, sf::Color color)
{
    if (size <= 0.0f || r.size.x <= 0.0f || r.size.y <= 0.0f) {
        return;
    }
    auto const inner = roundedOutline(r, radius, 0.0f);
    auto const outer = roundedOutline(r, radius, size);
    band(va, inner, outer, color, {color.r, color.g, color.b, 0});
}

void line(sf::VertexArray& va, V a, V b, float width, sf::Color color)
{
    V const d = b - a;
    float const l = length(d);
    if (l <= 0.0f) {
        return;
    }
    V const n = V{-d.y, d.x} * (0.5f * width / l);
    quad(va, a + n, b + n, b - n, a - n, color, color, color, color);
}

void circle(sf::VertexArray& va, V center, float radius, sf::Color color, unsigned segments)
{
    segments = std::max(segments, 3u);
    V prev = center + V{radius, 0.0f};
    for (unsigned i = 1; i <= segments; ++i) {
        float const a = TwoPi * static_cast<float>(i) / static_cast<float>(segments);
        V const p = center + V{std::cos(a), std::sin(a)} * radius;
        tri(va, center, color, prev, color, p, color);
        prev = p;
    }
}

void ring(sf::VertexArray& va, V center, float radius, float thickness, sf::Color color, unsigned segments)
{
    segments = std::max(segments, 3u);
    float const inner = std::max(0.0f, radius - thickness);
    for (unsigned i = 0; i < segments; ++i) {
        float const a0 = TwoPi * static_cast<float>(i) / static_cast<float>(segments);
        float const a1 = TwoPi * static_cast<float>(i + 1) / static_cast<float>(segments);
        V const u0{std::cos(a0), std::sin(a0)};
        V const u1{std::cos(a1), std::sin(a1)};
        quad(va, center + u0 * inner, center + u0 * radius, center + u1 * radius, center + u1 * inner,
             color, color, color, color);
    }
}

void arrow(sf::VertexArray& va, V from, V to, float width, float headSize, sf::Color color)
{
    V const d = to - from;
    float const l = length(d);
    if (l <= 0.0f) {
        return;
    }
    V const u = d / l;
    V const n{-u.y, u.x};
    float const head = std::min(headSize, l);
    V const base = to - u * head;
    line(va, from, base, width, color);
    tri(va, to, color, base + n * (0.5f * head), color, base - n * (0.5f * head), color);
}

void bezier(sf::VertexArray& va, V a, V b, V c, float width, sf::Color color, unsigned segments)
{
    segments = std::max(segments, 1u);
    std::vector<V> pts(segments + 1);
    for (unsigned i = 0; i <= segments; ++i) {
        float const t = static_cast<float>(i) / static_cast<float>(segments);
        float const u = 1.0f - t;
        pts[i] = a * (u * u) + b * (2.0f * u * t) + c * (t * t);
    }
    polyline(va, pts.data(), pts.size(), width, color);
}

void polyline(sf::VertexArray& va, V const* points, std::size_t count, float width, sf::Color color,
              float const* widths, sf::Color const* colors)
{
    // Drop repeated points: their direction is undefined.
    std::vector<std::size_t> idx;
    idx.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (idx.empty() || length2(points[i] - points[idx.back()]) > 1e-12f) {
            idx.push_back(i);
        }
    }
    std::size_t const n = idx.size();
    if (n < 2) {
        return;
    }

    auto dir = [&](std::size_t k) { return normalize(points[idx[k + 1]] - points[idx[k]]); };
    std::vector<V> left(n);
    std::vector<V> right(n);
    for (std::size_t k = 0; k < n; ++k) {
        float const w = 0.5f * (widths ? widths[idx[k]] : width);
        V const p = points[idx[k]];
        V offset;
        if (k == 0) {
            offset = normal(dir(0)) * w;
        } else if (k == n - 1) {
            offset = normal(dir(n - 2)) * w;
        } else {
            V const n0 = normal(dir(k - 1));
            V const n1 = normal(dir(k));
            V m = n0 + n1;
            float const ml = length(m);
            if (ml < 1e-6f) { // full U-turn
                m = n0;
            } else {
                m = m / ml;
            }
            float const cosHalf = std::max(dot(m, n0), 0.5f); // limit the miter to 2x
            offset = m * (w / cosHalf);
        }
        left[k]  = p + offset;
        right[k] = p - offset;
    }
    for (std::size_t k = 0; k + 1 < n; ++k) {
        sf::Color const c0 = colors ? colors[idx[k]] : color;
        sf::Color const c1 = colors ? colors[idx[k + 1]] : color;
        quad(va, left[k], left[k + 1], right[k + 1], right[k], c0, c1, c1, c0);
    }
}

void area(sf::VertexArray& va, V const* points, std::size_t count, float baseY, sf::Color const* topColors,
          sf::Color baseColor)
{
    for (std::size_t i = 0; i + 1 < count; ++i) {
        V const a = points[i];
        V const b = points[i + 1];
        quad(va, a, b, {b.x, baseY}, {a.x, baseY}, topColors[i], topColors[i + 1], baseColor, baseColor);
    }
}

} // namespace sml::geo
