#include "simplyml/ml/network_view.hpp"

#include <algorithm>
#include <cmath>
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
    if (m_footer) {
        m.footerH = t.px(24.0f) + 2.0f * capHeight(ctx.font, small) + t.px(8.0f);
    }
    return m;
}

sf::Vector2f NetworkView::naturalSize(UiContext const& ctx) const
{
    Theme const& t = ctx.theme;
    Metrics const m = metrics(ctx);
    float const chromeW = 2.0f * (t.px(t.outline) + t.padding().x);
    if (m.layers == 0) {
        return {chromeW + t.px(120.0f), chromeHeight(ctx) + t.px(60.0f)};
    }
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
    LayeredGraph const& g = m_input.get();
    if (!g.sameTopology(m_graph)) {
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

void NetworkView::onLayout(UiContext const& ctx)
{
    Panel::onLayout(ctx);
    place(ctx);
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

    for (auto const& e : m_graph.edges) {
        if (e.from < 0 || e.to < 0 || e.from >= n || e.to >= n) {
            continue;
        }
        float const     v     = e.value;
        sf::Color const color = v > 0.0f ? pal.nnPositive : v < 0.0f ? pal.nnNegative : pal.text;
        float const     width = std::clamp(std::abs(v) * m_edgeScale, 1.0f, NodeRadius) * t.px(1.0f) * zoom;
        geo::line(m_mesh, m_pos[static_cast<std::size_t>(e.from)], m_pos[static_cast<std::size_t>(e.to)],
                  std::max(width, 1.0f), color);
    }
    float const    ring     = t.px(Ring) * zoom;
    float const    outline  = t.px(Outline) * zoom;
    unsigned const segments = static_cast<unsigned>(std::clamp(m_radius * 2.0f, 8.0f, 32.0f));
    for (std::size_t i = 0; i < m_pos.size(); ++i) {
        float const v = m_graph.nodes[i].value;
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
    int const      last  = m_layerOf.empty() ? 0 : *std::max_element(m_layerOf.begin(), m_layerOf.end());
    float const    zoom  = m_radius / t.px(NodeRadius);
    float const    gap   = t.px(NodeRadius + Ring + Outline) * zoom + t.px(LabelGap);
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
        sf::FloatRect const c    = contentRect();
        float const         cap  = capHeight(ctx.font, small);
        float const         y    = m_footerY + t.px(24.0f);
        drawText(target, ctx.font, "Hidden nodes: " + std::to_string(hidden), small, {c.position.x, y}, t.palette.text);
        drawText(target, ctx.font, "Connections : " + std::to_string(m_graph.edges.size()), small,
                 {c.position.x, y + cap + t.px(8.0f)}, t.palette.text);
    }
}

} // namespace sml
