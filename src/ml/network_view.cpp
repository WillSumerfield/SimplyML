#include "simplyml/ml/network_view.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

#include "simplyml/ui/geometry.hpp"
#include "simplyml/ui/text.hpp"

namespace sml
{

namespace
{

// Unzoomed sizes from the original network renderer, in theme px.
constexpr float NodeRadius = 9.0f;
constexpr float Ring       = 3.0f;  // black gap between the value disc's max and the outline
constexpr float Outline    = 2.0f;
constexpr float Spacing    = 16.0f; // between neighbouring nodes
constexpr float LabelGap   = 10.0f;
constexpr float MaxNatural = 480.0f; // longest side of a Placed Graph's natural size, nodes only

} // namespace

NetworkView::NetworkView(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
{}

NetworkView::Metrics NetworkView::metrics(UiContext const& ctx) const
{
    Theme const&   t     = ctx.theme;
    unsigned const small = t.pt(t.textSmall);
    Metrics        m;
    std::vector<int> perLayer;
    for (std::size_t i = 0; i < m_graph.nodes.size(); ++i) {
        int const l = m_layerOf[i];
        if (l >= static_cast<int>(perLayer.size())) {
            perLayer.resize(static_cast<std::size_t>(l) + 1, 0);
        }
        ++perLayer[static_cast<std::size_t>(l)];
    }
    m.layers  = static_cast<int>(perLayer.size());
    m.tallest = perLayer.empty() ? 0 : *std::max_element(perLayer.begin(), perLayer.end());
    if (m.layers == 0) {
        return m;
    }
    for (std::size_t i = 0; i < m_graph.nodes.size(); ++i) {
        auto const& label = m_graph.nodes[i].label;
        if (label.empty()) {
            continue;
        }
        float const w = (m_vertical ? capHeight(ctx.font, small) : textSize(ctx.font, label, small).x) + t.px(LabelGap);
        if (m_layerOf[i] == 0) {
            m.labelBefore = std::max(m.labelBefore, w);
        } else if (m_layerOf[i] == m.layers - 1) {
            m.labelAfter = std::max(m.labelAfter, w);
        }
    }
    float const pitch = t.px(2.0f * NodeRadius + Spacing);
    float const node  = 2.0f * t.px(NodeRadius + Ring + Outline);
    m.graphL = static_cast<float>(m.layers - 1) * pitch + node;
    m.graphN = static_cast<float>(m.tallest - 1) * pitch + node;
    m.footerH = footerHeight(ctx);
    return m;
}

float NetworkView::footerHeight(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    return m_footer ? t.px(24.0f) + 2.0f * capHeight(ctx.font, t.pt(t.textSmall)) + t.px(8.0f) : 0.0f;
}

NetworkView::Bands NetworkView::bands(UiContext const& ctx) const
{
    Theme const&   t     = ctx.theme;
    unsigned const small = t.pt(t.textSmall);
    Bands          b;
    for (std::size_t i = 0; i < m_placed.nodes.size(); ++i) {
        auto const& label = m_placed.nodes[i].label;
        if (label.empty()) {
            continue;
        }
        switch (m_side[i]) {
        case Side::Left:  b.left  = std::max(b.left, textSize(ctx.font, label, small).x + t.px(LabelGap)); break;
        case Side::Right: b.right = std::max(b.right, textSize(ctx.font, label, small).x + t.px(LabelGap)); break;
        case Side::Above: b.above = std::max(b.above, capHeight(ctx.font, small) + t.px(LabelGap)); break;
        case Side::Below: b.below = std::max(b.below, capHeight(ctx.font, small) + t.px(LabelGap)); break;
        }
    }
    return b;
}

sf::Vector2f NetworkView::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    float const chromeW = 2.0f * (t.px(t.outline) + t.padding().x);
    if (nodeCount() == 0) {
        return {chromeW + t.px(120.0f), chromeHeight(ctx) + t.px(60.0f)};
    }
    if (m_isPlaced) {
        // Scale so the closest pair sits one pitch apart at zoom 1, capped so one tight pair can't
        // ask for a huge panel.
        Bands const b    = bands(ctx);
        float const node = 2.0f * t.px(NodeRadius + Ring + Outline);
        float       s    = m_minDist > 0.0f ? t.px(2.0f * NodeRadius + Spacing) / m_minDist : 0.0f;
        float const longest = std::max(m_size.x, m_size.y) * s;
        if (longest > t.px(MaxNatural)) {
            s *= t.px(MaxNatural) / longest;
        }
        return {chromeW + b.left + m_size.x * s + node + b.right,
                chromeHeight(ctx) + b.above + m_size.y * s + node + b.below + footerHeight(ctx)};
    }
    Metrics const m = metrics(ctx);
    float const along = m.labelBefore + m.graphL + m.labelAfter;
    if (m_vertical) {
        return {chromeW + m.graphN, chromeHeight(ctx) + along + m.footerH};
    }
    return {chromeW + along, chromeHeight(ctx) + m.graphN + m.footerH};
}

void NetworkView::update(UiContext const& ctx)
{
    Panel::update(ctx);
    if (m_input.version() == m_seen) {
        return;
    }
    m_seen = m_input.version();
    auto const& in = m_input.get();
    if (auto const* p = std::get_if<PlacedGraph>(&in)) {
        if (!m_isPlaced || !p->sameTopology(m_placed)) {
            m_isPlaced = true;
            m_graph    = {};
            m_layerOf.clear();
            m_placed = *p;
            m_pos.clear(); // stale until the next layout
            frame();
            invalidate();
        } else {
            for (std::size_t k = 0; k < p->nodes.size(); ++k) {
                m_placed.nodes[k].value = p->nodes[k].value;
            }
            for (std::size_t k = 0; k < p->edges.size(); ++k) {
                m_placed.edges[k].value = p->edges[k].value;
            }
        }
        m_meshDirty = true;
        return;
    }
    LayeredGraph const& g = std::get<LayeredGraph>(in);
    if (m_isPlaced || !g.sameTopology(m_graph)) {
        m_isPlaced = false;
        m_placed   = {};
        m_side.clear();
        m_graph = g;
        std::map<int, int> rank; // sparse layer ids (e.g. NEAT depths) -> 0..n-1
        for (auto const& n : m_graph.nodes) {
            rank[n.layer] = 0;
        }
        int i = 0;
        for (auto& [layer, r] : rank) {
            r = i++;
        }
        m_layerOf.clear();
        for (auto const& n : m_graph.nodes) {
            m_layerOf.push_back(rank[n.layer]);
        }
        m_pos.clear(); // stale until the next layout
        invalidate();
    } else {
        for (std::size_t k = 0; k < g.nodes.size(); ++k) {
            m_graph.nodes[k].value = g.nodes[k].value;
        }
        for (std::size_t k = 0; k < g.edges.size(); ++k) {
            m_graph.edges[k].value = g.edges[k].value;
        }
    }
    m_meshDirty = true;
}

void NetworkView::frame()
{
    auto const& nodes = m_placed.nodes;
    m_min = m_size = {};
    m_minDist      = 0.0f;
    m_side.assign(nodes.size(), Side::Right);
    if (nodes.empty()) {
        return;
    }
    sf::Vector2f lo = nodes[0].position, hi = lo;
    for (auto const& n : nodes) {
        lo = {std::min(lo.x, n.position.x), std::min(lo.y, n.position.y)};
        hi = {std::max(hi.x, n.position.x), std::max(hi.y, n.position.y)};
    }
    m_min  = lo;
    m_size = hi - lo;
    // Closest pair, ignoring coincident nodes (they overlap whatever the zoom). O(n^2), but only on
    // topology changes.
    float best = std::numeric_limits<float>::infinity();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        for (std::size_t k = i + 1; k < nodes.size(); ++k) {
            float const d2 = (nodes[i].position - nodes[k].position).lengthSquared();
            if (d2 > 0.0f && d2 < best) {
                best = d2;
            }
        }
    }
    m_minDist = std::isfinite(best) ? std::sqrt(best) : 0.0f;
    sf::Vector2f const mid  = lo + 0.5f * m_size;
    bool const         wide = m_size.x >= m_size.y;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        sf::Vector2f const p = nodes[i].position;
        m_side[i] = wide ? (p.x < mid.x ? Side::Left : Side::Right) : (p.y < mid.y ? Side::Above : Side::Below);
    }
}

void NetworkView::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    if (m_isPlaced) {
        placePlaced(ctx);
    } else {
        place(ctx);
    }
}

void NetworkView::placePlaced(UiContext const& ctx)
{
    Theme const&        t = ctx.theme;
    sf::FloatRect const c = contentRect();
    m_pos.assign(m_placed.nodes.size(), {});
    m_meshDirty = true;
    if (m_placed.nodes.empty()) {
        return;
    }
    Bands const b       = bands(ctx);
    float const footerH = footerHeight(ctx);
    float const availW  = c.size.x - b.left - b.right;
    float const availH  = c.size.y - b.above - b.below - footerH;
    float const pitch   = t.px(2.0f * NodeRadius + Spacing);
    float const edge    = t.px(NodeRadius + Ring + Outline); // node center to outer edge, unzoomed
    float const inf     = std::numeric_limits<float>::infinity();
    // Zoom z and scale s (px per user unit). The closest pair must stay one pitch apart, so
    // z <= minDist * s / pitch; the box plus a node's edge on each side must fit:
    // size * s + 2 * edge * z <= avail. Take the biggest s with z tied to it, then cap z at max zoom.
    float z = 0.0f;
    if (m_minDist > 0.0f) {
        float const k = m_minDist / pitch;
        float const s = std::min(availW / (m_size.x + 2.0f * edge * k), availH / (m_size.y + 2.0f * edge * k));
        z = std::min(m_maxZoom, k * s);
    } else {
        z = std::min({m_maxZoom, availW / (2.0f * edge), availH / (2.0f * edge)});
    }
    z = std::max(0.05f, z);
    float const sw = m_size.x > 0.0f ? (availW - 2.0f * edge * z) / m_size.x : inf;
    float const sh = m_size.y > 0.0f ? (availH - 2.0f * edge * z) / m_size.y : inf;
    float       s  = std::max(0.0f, std::min(sw, sh));
    if (!std::isfinite(s)) {
        s = 0.0f; // all nodes coincide
    }
    float const gw = m_size.x * s + 2.0f * edge * z;
    float const gh = m_size.y * s + 2.0f * edge * z;
    float const x0 = c.position.x + 0.5f * (c.size.x - (b.left + gw + b.right)) + b.left + edge * z;
    float const y0 = c.position.y + 0.5f * (c.size.y - (b.above + gh + b.below + footerH)) + b.above + edge * z;
    for (std::size_t i = 0; i < m_pos.size(); ++i) {
        sf::Vector2f const p = m_placed.nodes[i].position - m_min;
        m_pos[i] = {x0 + p.x * s, y0 + p.y * s};
    }
    m_center  = {x0 + 0.5f * m_size.x * s, y0 + 0.5f * m_size.y * s};
    m_radius  = t.px(NodeRadius) * z;
    m_footerY = y0 - edge * z + gh + b.below;
}

void NetworkView::place(UiContext const& ctx)
{
    Theme const&        t = ctx.theme;
    Metrics const       m = metrics(ctx);
    sf::FloatRect const c = contentRect();
    m_pos.assign(m_graph.nodes.size(), {});
    m_meshDirty = true;
    if (m.layers == 0) {
        return;
    }
    // Layout in (L, N): L runs across layers, N along a layer; the footer sits below either way.
    bool const  vert   = m_vertical;
    float const posL   = vert ? c.position.y : c.position.x;
    float const posN   = vert ? c.position.x : c.position.y;
    float const sizeL  = (vert ? c.size.y : c.size.x) - (vert ? m.footerH : 0.0f);
    float const sizeN  = (vert ? c.size.x : c.size.y) - (vert ? 0.0f : m.footerH);
    float const availL = sizeL - m.labelBefore - m.labelAfter;
    float const zoom   = std::max(0.05f, std::min({m_maxZoom, availL / m.graphL, sizeN / m.graphN}));
    float const pitch = t.px(2.0f * NodeRadius + Spacing) * zoom;
    float const edge  = t.px(NodeRadius + Ring + Outline) * zoom;
    // Spare room spreads the layers apart, up to 3x the pitch at max zoom: big layers shrink the node
    // pitch, but shouldn't also squash the gaps between layers.
    float const layersL = static_cast<float>(m.layers - 1);
    float const spreadL = std::max(pitch, 3.0f * t.px(2.0f * NodeRadius + Spacing) * m_maxZoom);
    float const pitchL  = m.layers > 1 ? std::clamp((availL - 2.0f * edge) / layersL, pitch, spreadL) : pitch;
    float const graphL  = layersL * pitchL + 2.0f * edge;
    float const blockL  = m.labelBefore + graphL + m.labelAfter + (vert ? m.footerH : 0.0f);
    float const blockN  = m.graphN * zoom + (vert ? 0.0f : m.footerH);
    float const gl      = posL + 0.5f * ((vert ? c.size.y : c.size.x) - blockL) + m.labelBefore;
    float const gn      = posN + 0.5f * ((vert ? c.size.x : c.size.y) - blockN);

    std::vector<int> count(static_cast<std::size_t>(m.layers), 0);
    for (int l : m_layerOf) {
        ++count[static_cast<std::size_t>(l)];
    }
    std::vector<int> seen(count.size(), 0);
    for (std::size_t i = 0; i < m_graph.nodes.size(); ++i) {
        auto const  l   = static_cast<std::size_t>(m_layerOf[i]);
        float const off = 0.5f * static_cast<float>(m.tallest - count[l]) * pitch;
        float const u   = gl + edge + static_cast<float>(l) * pitchL;
        float const v   = gn + edge + off + static_cast<float>(seen[l]++) * pitch;
        m_pos[i] = vert ? sf::Vector2f{v, u} : sf::Vector2f{u, v};
    }
    m_radius  = t.px(NodeRadius) * zoom;
    m_footerY = vert ? gl + graphL + m.labelAfter : gn + m.graphN * zoom;
}

void NetworkView::rebuildMesh(UiContext const& ctx)
{
    m_meshDirty = false;
    m_mesh.clear();
    Theme const&   t    = ctx.theme;
    Palette const& pal  = t.palette;
    float const    zoom = m_radius / t.px(NodeRadius);
    int const      n    = static_cast<int>(m_pos.size());

    float const    ring     = t.px(Ring) * zoom;
    float const    outline  = t.px(Outline) * zoom;
    float const    outer    = m_radius + ring + outline;
    unsigned const segments = static_cast<unsigned>(std::clamp(m_radius * 2.0f, 8.0f, 32.0f));
    for (auto const& e : edges()) {
        if (e.from < 0 || e.to < 0 || e.from >= n || e.to >= n) {
            continue;
        }
        float const     v     = e.value;
        sf::Color const color = v > 0.0f ? pal.nnPositive : v < 0.0f ? pal.nnNegative : pal.text;
        float const     width = std::max(std::clamp(std::abs(v) * m_edgeScale, 1.0f, NodeRadius) * t.px(1.0f) * zoom, 1.0f);
        sf::Vector2f const a = m_pos[static_cast<std::size_t>(e.from)];
        if (e.from != e.to) {
            geo::line(m_mesh, a, m_pos[static_cast<std::size_t>(e.to)], width, color);
            continue;
        }
        // Self-loop: a ring poking out of the node, away from the graph (placed) or off the layer axis.
        sf::Vector2f dir = m_isPlaced ? a - m_center : m_vertical ? sf::Vector2f{-1.0f, 0.0f} : sf::Vector2f{0.0f, -1.0f};
        dir              = dir.lengthSquared() > 1e-6f ? dir.normalized() : sf::Vector2f{0.0f, -1.0f};
        float const loop = 0.6f * outer;
        float const w    = std::min(width, 0.5f * loop);
        geo::ring(m_mesh, a + dir * (outer + 0.3f * loop), loop + 0.5f * w, w, color, segments);
    }
    for (std::size_t i = 0; i < m_pos.size(); ++i) {
        float const v = nodeValue(i);
        geo::circle(m_mesh, m_pos[i], m_radius + ring + outline, pal.text, segments);
        geo::circle(m_mesh, m_pos[i], m_radius + ring, sf::Color::Black, segments);
        float const r = std::min(std::abs(v), 1.0f) * m_radius;
        if (r > 0.25f) {
            geo::circle(m_mesh, m_pos[i], r, v > 0.0f ? pal.nnPositive : pal.nnNegative, segments);
        }
    }
    if (m_footer && !m_pos.empty()) {
        sf::FloatRect const c = contentRect();
        geo::rect(m_mesh, {{c.position.x, m_footerY + t.px(12.0f)}, {c.size.x, t.px(1.0f)}}, pal.text);
    }
}

void NetworkView::drawContent(sf::RenderTarget& target, UiContext const& ctx)
{
    if (m_meshDirty) {
        rebuildMesh(ctx);
    }
    target.draw(m_mesh);
    if (m_pos.empty()) {
        return;
    }
    Theme const&   t     = ctx.theme;
    unsigned const small = t.pt(t.textSmall);
    float const    zoom  = m_radius / t.px(NodeRadius);
    float const    gap   = t.px(NodeRadius + Ring + Outline) * zoom + t.px(LabelGap);
    sf::FloatRect const c   = contentRect();
    float const         cap = capHeight(ctx.font, small);
    float const         y   = m_footerY + t.px(24.0f);
    if (m_isPlaced) {
        for (std::size_t i = 0; i < m_pos.size(); ++i) {
            auto const&        label = m_placed.nodes[i].label;
            sf::Vector2f const p     = m_pos[i];
            if (label.empty()) {
                continue;
            }
            switch (m_side[i]) {
            case Side::Left:  drawText(target, ctx.font, label, small, {p.x - gap, p.y}, t.palette.text, Align::End, Align::Center); break;
            case Side::Right: drawText(target, ctx.font, label, small, {p.x + gap, p.y}, t.palette.text, Align::Start, Align::Center); break;
            case Side::Above: drawText(target, ctx.font, label, small, {p.x, p.y - gap}, t.palette.text, Align::Center, Align::End); break;
            case Side::Below: drawText(target, ctx.font, label, small, {p.x, p.y + gap}, t.palette.text, Align::Center, Align::Start); break;
            }
        }
        if (m_footer) {
            drawText(target, ctx.font, "Nodes       : " + std::to_string(m_placed.nodes.size()), small, {c.position.x, y},
                     t.palette.text);
            drawText(target, ctx.font, "Connections : " + std::to_string(m_placed.edges.size()), small,
                     {c.position.x, y + cap + t.px(8.0f)}, t.palette.text);
        }
        return;
    }
    int const last = m_layerOf.empty() ? 0 : *std::max_element(m_layerOf.begin(), m_layerOf.end());
    for (std::size_t i = 0; i < m_pos.size(); ++i) {
        auto const& label = m_graph.nodes[i].label;
        if (label.empty() || (m_layerOf[i] != 0 && m_layerOf[i] != last)) {
            continue;
        }
        bool const before = m_layerOf[i] == 0;
        float const d = before ? -gap : gap;
        Align const a = before ? Align::End : Align::Start;
        if (m_vertical) {
            drawText(target, ctx.font, label, small, {m_pos[i].x, m_pos[i].y + d}, t.palette.text, Align::Center, a);
        } else {
            drawText(target, ctx.font, label, small, {m_pos[i].x + d, m_pos[i].y}, t.palette.text, a, Align::Center);
        }
    }
    if (m_footer) {
        int hidden = 0;
        for (int l : m_layerOf) {
            hidden += (l != 0 && l != last) ? 1 : 0;
        }
        drawText(target, ctx.font, "Hidden nodes: " + std::to_string(hidden), small, {c.position.x, y}, t.palette.text);
        drawText(target, ctx.font, "Connections : " + std::to_string(m_graph.edges.size()), small,
                 {c.position.x, y + cap + t.px(8.0f)}, t.palette.text);
    }
}

} // namespace sml
