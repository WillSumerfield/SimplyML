#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/core/snapshot.hpp"
#include "simplyml/ml/formats.hpp"
#include "simplyml/ui/panel.hpp"

namespace sml
{

/// Panel drawing a LayeredGraph: one column per layer, each centered vertically; nodes as rings
/// filled by |value| (green positive, red negative), edges as lines whose width follows |value|.
/// First-layer labels go on the left, last-layer labels on the right, and a footer counts hidden
/// nodes and connections. Node positions are cached and only recomputed when the topology changes.
class NetworkView : public Panel
{
public:
    explicit NetworkView(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    /// Thread-safe: call from the training thread; the view picks it up next frame.
    void setGraph(LayeredGraph graph) { m_input.set(std::move(graph)); }

    /// Edge width in px at scale 1 is |value| * scale, clamped to [1, node radius] (default 20).
    NetworkView& setEdgeScale(float scale) { m_edgeScale = scale; m_meshDirty = true; return *this; }
    NetworkView& setFooter(bool footer) { m_footer = footer; invalidate(); return *this; }
    /// Largest zoom when the panel is bigger than the graph's natural size (default 1.5).
    NetworkView& setMaxZoom(float zoom) { m_maxZoom = zoom; invalidate(); return *this; }

    /// Laid-out graph (UI thread).
    [[nodiscard]] LayeredGraph const& graph() const { return m_graph; }
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
        float labelLeft = 0.0f, labelRight = 0.0f; // label columns incl. gap
        float graphW = 0.0f, graphH = 0.0f;         // nodes only, unzoomed
        float footerH = 0.0f;
        int   layers = 0, tallest = 0;
    };
    [[nodiscard]] Metrics metrics(UiContext const& ctx) const;
    void place(UiContext const& ctx);
    void rebuildMesh(UiContext const& ctx);

    Snapshot<LayeredGraph> m_input;
    std::uint64_t          m_seen = 0;
    LayeredGraph           m_graph;
    std::vector<int>       m_layerOf; // normalized layer per node (0..layers-1)

    float m_edgeScale = 20.0f;
    float m_maxZoom   = 1.5f;
    bool  m_footer    = true;

    std::vector<sf::Vector2f> m_pos;
    float                     m_radius = 0.0f;
    float                     m_footerY = 0.0f;
    bool                      m_meshDirty = true;
    sf::VertexArray           m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
