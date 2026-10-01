#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/ui/axes.hpp"
#include "simplyml/ui/panel.hpp"
#include "simplyml/ui/value_format.hpp"

namespace sml
{

/// Line chart of one or more Metric Store series against their steps, in a Panel.
/// One series: area fill and the latest value as the readout. Several: a legend instead.
/// Geometry is rebuilt only when data, size or the (smoothly animated) y range change; long
/// series are reduced to min/max per pixel column, so cost doesn't grow with history.
class LineChart : public Panel
{
public:
    explicit LineChart(std::string title = {}, std::string series = {}, sf::Color color = sf::Color::Transparent);

    /// `color` transparent = the panel accent for the first series, else the palette cycle.
    LineChart& addSeries(std::string name, sf::Color color = sf::Color::Transparent, std::string label = {});
    /// Shows only the newest `points` of each series (0 = all, the default).
    LineChart& setWindow(std::size_t points)      { m_window = points; m_dirty = true; return *this; }
    /// Fill under the line (single-series charts only; default on).
    LineChart& setArea(bool area)                 { m_area = area; m_dirty = true; return *this; }
    /// Fixed y range; `setAutoY` returns to fitting the data.
    LineChart& setYRange(double lo, double hi);
    LineChart& setAutoY(bool includeZero = true);
    LineChart& setYFormat(ValueFormat fmt)        { m_yFormat = std::move(fmt); m_yFormatSet = true; return *this; }
    LineChart& setXFormat(ValueFormat fmt)        { m_xFormat = std::move(fmt); return *this; }
    LineChart& setValueFormat(ValueFormat fmt)    { m_valueFormat = std::move(fmt); return *this; }

    [[nodiscard]] std::size_t seriesCount() const { return m_lines.size(); }

    void update(UiContext const& ctx) override;

protected:
    void onLayout(UiContext const& ctx) override;
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    struct Line
    {
        std::string   name;
        std::string   label;
        sf::Color     color;
        std::uint64_t seenTotal = UINT64_MAX;
        std::size_t   seenSize  = 0;
    };

    [[nodiscard]] sf::Color colorOf(std::size_t i, Theme const& theme) const;
    void rebuild(UiContext const& ctx);

    std::vector<Line> m_lines;
    std::size_t       m_window = 0;
    bool              m_area   = true;
    bool              m_autoY  = true;
    bool              m_includeZero = true;
    double            m_fixedLo = 0.0, m_fixedHi = 1.0;
    ValueFormat       m_yFormat;
    bool              m_yFormatSet = false;
    ValueFormat       m_xFormat = ValueFormat::integer();
    ValueFormat       m_valueFormat = ValueFormat::number(4);

    SmoothRange         m_range;
    Ticks               m_yTicks;
    std::vector<double> m_xTicks;
    AxisMap             m_map;
    bool                m_hasData = false;
    bool                m_dirty   = true;
    sf::VertexArray     m_grid{sf::PrimitiveType::Triangles};
    sf::VertexArray     m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
