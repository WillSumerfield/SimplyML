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
        float const w = textSize(ctx.font, label, small).x + t.px(LabelGap);
        if (m_layerOf[i] == 0) {
            m.labelLeft = std::max(m.labelLeft, w);
        } else if (m_layerOf[i] == m.layers - 1) {
            m.labelRight = std::max(m.labelRight, w);
        }
    }
    float const pitch = t.px(2.0f * NodeRadius + Spacing);
    float const node  = 2.0f * t.px(NodeRadius + Ring + Outline);
    m.graphW = static_cast<float>(m.layers - 1) * pitch + node;
    m.graphH = static_cast<float>(m.tallest - 1) * pitch + node;
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
    return {chromeW + m.labelLeft + m.graphW + m.labelRight, chromeHeight(ctx) + m.graphH + m.footerH};
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
    float const availW = c.size.x - m.labelLeft - m.labelRight;
    float const availH = c.size.y - m.footerH;
    float const zoom   = std::max(0.05f, std::min({m_maxZoom, availW / m.graphW, availH / m.graphH}));
    float const pitch = t.px(2.0f * NodeRadius + Spacing) * zoom;
    float const edge  = t.px(NodeRadius + Ring + Outline) * zoom;
    // Spare width spreads the layers apart (up to 3x), so wide panels don't leave a narrow graph.
    float const layersW = static_cast<float>(m.layers - 1);
    float const pitchX  = m.layers > 1 ? std::clamp((availW - 2.0f * edge) / layersW, pitch, 3.0f * pitch) : pitch;
    float const graphW  = layersW * pitchX + 2.0f * edge;
    float const blockW  = m.labelLeft + graphW + m.labelRight;
    float const blockH  = m.graphH * zoom + m.footerH;
    float const gx      = c.position.x + 0.5f * (c.size.x - blockW) + m.labelLeft;
    float const gy      = c.position.y + 0.5f * (c.size.y - blockH);

    std::vector<int> count(static_cast<std::size_t>(m.layers), 0);
    for (int l : m_layerOf) {
        ++count[static_cast<std::size_t>(l)];
    }
    std::vector<int> seen(count.size(), 0);
    for (std::size_t i = 0; i < m_graph.nodes.size(); ++i) {
        auto const  l   = static_cast<std::size_t>(m_layerOf[i]);
        float const off = 0.5f * static_cast<float>(m.tallest - count[l]) * pitch;
        m_pos[i] = {gx + edge + static_cast<float>(l) * pitchX, gy + edge + off + static_cast<float>(seen[l]++) * pitch};
    }
    m_radius  = t.px(NodeRadius) * zoom;
    m_footerY = gy + m.graphH * zoom;
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
        bool const left = m_layerOf[i] == 0;
        drawText(target, ctx.font, label, small, {m_pos[i].x + (left ? -gap : gap), m_pos[i].y}, t.palette.text,
                 left ? Align::End : Align::Start, Align::Center);
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
