#pragma once
#include <cstdint>
#include <string>

#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/ui/axes.hpp"
#include "simplyml/ui/panel.hpp"

namespace sml
{

/// XY trajectory from two Metric Store series paired by index (e.g. angle vs angular velocity),
/// in a Panel. The newest points are drawn thickest and fade thinner with age; at most
/// `maxPoints` (default 500) are kept on screen.
class PhasePlot : public Panel
{
public:
    PhasePlot(std::string title = {}, std::string xSeries = {}, std::string ySeries = {},
              sf::Color color = sf::Color::Transparent);

    PhasePlot& setSeries(std::string x, std::string y);
    PhasePlot& setMaxPoints(std::size_t points);
    /// Fixed ranges; by default each axis fits the data (and never shrinks below +-1e-6).
    PhasePlot& setRanges(double xlo, double xhi, double ylo, double yhi);
    PhasePlot& setAutoRanges();
    /// Line width (px) of the newest and oldest points.
    PhasePlot& setWidths(float newest, float oldest) { m_newest = newest; m_oldest = oldest; m_dirty = true; return *this; }

    void update(UiContext const& ctx) override;

protected:
    void onLayout(UiContext const& ctx) override;
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    void rebuild(UiContext const& ctx);

    std::string   m_x, m_y;
    std::size_t   m_maxPoints = 500;
    std::uint64_t m_seenX = UINT64_MAX, m_seenY = UINT64_MAX;
    bool          m_auto  = true;
    double        m_xlo = -1.0, m_xhi = 1.0, m_ylo = -1.0, m_yhi = 1.0;
    float         m_newest = 4.0f, m_oldest = 1.0f;
    AxisMap       m_map;
    bool          m_dirty = true;
    sf::VertexArray m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
