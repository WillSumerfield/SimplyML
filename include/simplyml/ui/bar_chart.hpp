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

/// Bars for the newest values of one Metric Store series (e.g. score per generation), with a
/// translucent body and a solid cap, in a Panel. Shares its axes with LineChart.
class BarChart : public Panel
{
public:
    explicit BarChart(std::string title = {}, std::string series = {}, sf::Color color = sf::Color::Transparent);

    BarChart& setSeries(std::string name) { m_series = std::move(name); m_seen = UINT64_MAX; return *this; }
    /// Newest `bars` values (default 50).
    BarChart& setWindow(std::size_t bars);
    /// Bars share the full width (they thin out as data arrives) instead of keeping window-sized slots.
    BarChart& setFill(bool fill) { m_fill = fill; m_dirty = true; return *this; }
    BarChart& setYRange(double lo, double hi);
    BarChart& setAutoY();
    BarChart& setYFormat(ValueFormat fmt)     { m_yFormat = std::move(fmt); m_yFormatSet = true; return *this; }
    BarChart& setXFormat(ValueFormat fmt)     { m_xFormat = std::move(fmt); return *this; }
    BarChart& setValueFormat(ValueFormat fmt) { m_valueFormat = std::move(fmt); return *this; }

    void update(UiContext const& ctx) override;

protected:
    void onLayout(UiContext const& ctx) override;
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    void rebuild(UiContext const& ctx);

    std::string   m_series;
    std::size_t   m_window = 50;
    std::uint64_t m_seen   = UINT64_MAX;
    bool          m_autoY  = true;
    bool          m_fill   = false;
    double        m_fixedLo = 0.0, m_fixedHi = 1.0;
    ValueFormat   m_yFormat;
    bool          m_yFormatSet = false;
    ValueFormat   m_xFormat     = ValueFormat::integer();
    ValueFormat   m_valueFormat = ValueFormat::number(4);

    SmoothRange         m_range;
    Ticks               m_yTicks;
    AxisMap             m_map;
    std::vector<double> m_xLabels; // steps of labelled bars
    std::vector<float>  m_xLabelPos;
    bool                m_dirty = true;
    sf::VertexArray     m_grid{sf::PrimitiveType::Triangles};
    sf::VertexArray     m_mesh{sf::PrimitiveType::Triangles};
};

} // namespace sml
