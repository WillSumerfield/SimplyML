#pragma once
#include <algorithm>
#include <cstdint>
#include <variant>
#include <vector>

#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/core/snapshot.hpp"
#include "simplyml/ml/formats.hpp"
#include "simplyml/ui/panel.hpp"

namespace sml
{

/// Panel drawing a LayeredGraph or a PlacedGraph; nodes as rings filled by |value| (green positive,
/// red negative), edges as lines whose width follows |value|, self-loops as small rings outside the
/// node. Node positions are cached and only recomputed when the topology changes.
///
/// Layered: one column per layer, each centered vertically (or, when vertical, one row per layer top
/// to bottom). First-layer labels go before the first layer (left / above), last-layer labels after
/// the last (right / below), and a footer counts hidden nodes and connections.
///
/// Placed: the nodes' bounding box is fitted to the panel with aspect kept; nodes shrink so the
/// closest pair keeps a gap. Every label goes on the node's side facing away from the box center,
/// along the box's longer axis, and the footer counts nodes and connections.
class NetworkView : public Panel
{
public:
    explicit NetworkView(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    /// Thread-safe: call from the training thread; the view picks it up next frame.
    void setGraph(LayeredGraph graph) { m_input.set(std::move(graph)); }
    void setGraph(PlacedGraph graph) { m_input.set(std::move(graph)); }

    /// Edge width in px at zoom 1 is |value| * scale, clamped to the edge width range (default 20).
    NetworkView& setEdgeScale(float scale) { m_edgeScale = scale; m_meshDirty = true; return *this; }
    /// Edge width range as fractions of the node radius, each in [0, 1] (default 0.1 to 1).
    NetworkView& setEdgeWidth(float min, float max)
    {
        m_edgeMin = std::clamp(min, 0.0f, 1.0f);
        m_edgeMax = std::clamp(max, m_edgeMin, 1.0f);
        m_meshDirty = true;
        return *this;
    }
    /// Edge opacity, 0-255 (default 255); lower it to see through dense networks.
    NetworkView& setEdgeAlpha(std::uint8_t alpha) { m_edgeAlpha = alpha; m_meshDirty = true; return *this; }
    NetworkView& setFooter(bool footer) { m_footer = footer; invalidate(); return *this; }
    /// Largest zoom when the panel is bigger than the graph's natural size (default 1.5).
    NetworkView& setMaxZoom(float zoom) { m_maxZoom = zoom; invalidate(); return *this; }
    /// Layers as rows from top to bottom instead of columns from left to right (default false).
    /// Placed Graphs ignore it.
    NetworkView& setVertical(bool vertical) { m_vertical = vertical; invalidate(); return *this; }

    /// Whether the laid-out graph is a PlacedGraph (UI thread).
    [[nodiscard]] bool isPlaced() const { return m_isPlaced; }
    /// Laid-out graph (UI thread); the other kind is empty.
    [[nodiscard]] LayeredGraph const& graph() const { return m_graph; }
    [[nodiscard]] PlacedGraph const&  placedGraph() const { return m_placed; }
    /// Node centers in screen px, after the last layout (UI thread).
    [[nodiscard]] std::vector<sf::Vector2f> const& nodePositions() const { return m_pos; }
    [[nodiscard]] float nodeRadius() const { return m_radius; }

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void update(UiContext const& ctx) override;

protected:
    void onLayout(UiContext const& ctx) override;
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    struct Metrics
    {
        float labelBefore = 0.0f, labelAfter = 0.0f; // label bands along the layer axis, incl. gap
        float graphL = 0.0f, graphN = 0.0f;          // nodes only, unzoomed: along layers / along a layer
        float footerH = 0.0f;
        int   layers = 0, tallest = 0;
    };
    enum class Side : std::uint8_t { Left, Right, Above, Below };
    struct Bands // placed label room on each side, incl. gap
    {
        float left = 0.0f, right = 0.0f, above = 0.0f, below = 0.0f;
    };
    [[nodiscard]] Metrics metrics(UiContext const& ctx) const;
    [[nodiscard]] Bands   bands(UiContext const& ctx) const;
    [[nodiscard]] float   footerHeight(UiContext const& ctx) const;
    [[nodiscard]] std::vector<GraphEdge> const& edges() const { return m_isPlaced ? m_placed.edges : m_graph.edges; }
    [[nodiscard]] std::size_t nodeCount() const { return m_isPlaced ? m_placed.nodes.size() : m_graph.nodes.size(); }
    [[nodiscard]] float       nodeValue(std::size_t i) const
    {
        return m_isPlaced ? m_placed.nodes[i].value : m_graph.nodes[i].value;
    }
    void frame(); // placed: bounding box, closest pair, label sides
    void place(UiContext const& ctx);
    void placePlaced(UiContext const& ctx);
    void rebuildMesh(UiContext const& ctx);

    Snapshot<std::variant<LayeredGraph, PlacedGraph>> m_input;
    std::uint64_t          m_seen = 0;
    bool                   m_isPlaced = false;
    LayeredGraph           m_graph;
    std::vector<int>       m_layerOf; // normalized layer per node (0..layers-1)
    PlacedGraph            m_placed;
    sf::Vector2f           m_min, m_size;     // placed bounding box, user units
    float                  m_minDist = 0.0f;  // placed closest pair at distinct positions (0 = none)
    std::vector<Side>      m_side;            // placed label side per node

    float m_edgeScale = 20.0f;
    float m_edgeMin   = 0.1f; // fractions of the node radius
    float m_edgeMax   = 1.0f;
    std::uint8_t m_edgeAlpha = 255;
    float m_maxZoom   = 1.5f;
    bool  m_footer    = true;
    bool  m_vertical  = false;

    std::vector<sf::Vector2f> m_pos;
    float                     m_radius = 0.0f;
    float                     m_footerY = 0.0f;
    sf::Vector2f              m_center; // placed bounding box center in px (self-loops point away from it)
    bool                      m_meshDirty = true;
    sf::VertexArray           m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
