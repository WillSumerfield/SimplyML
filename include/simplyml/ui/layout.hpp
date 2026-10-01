#pragma once
#include <vector>

#include "simplyml/ui/align.hpp"
#include "simplyml/ui/widget.hpp"

namespace sml
{

/// Splits `total` px between `lengths`: fixed parts first, the rest by `fr` share, `gap` between.
/// Returns each part's start offset and size. Parts never go negative.
struct Span1D
{
    float start = 0.0f;
    float size  = 0.0f;
};
[[nodiscard]] std::vector<Span1D> distribute(std::vector<Length> const& lengths, float total, float gap,
                                             Align align = Align::Start);

/// Children side by side (Row) or stacked (Column), sized along the main axis by their `extent`
/// and stretched across it. When nothing uses `fr`, leftover space is placed by `align`.
class Stack : public Container
{
public:
    enum class Axis
    {
        Horizontal,
        Vertical,
    };

    explicit Stack(Axis axis)
        : m_axis{axis}
    {}

    /// Empty space inside the edges; negative = theme margin (default 0).
    Stack& setPadding(float padding) { m_padding = padding; invalidate(); return *this; }
    /// Space between children; negative = theme gap (the default).
    Stack& setGap(float gap)         { m_gap = gap; invalidate(); return *this; }
    Stack& setAlign(Align align)     { m_align = align; invalidate(); return *this; }

    /// Visible children end to end (fixed px or natural size) plus gaps and padding along the axis;
    /// the largest natural size across it.
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;

protected:
    void arrange(UiContext const& ctx) override;

private:
    Axis  m_axis;
    float m_padding = 0.0f;
    float m_gap     = -1.0f;
    Align m_align   = Align::Start;
};

class Row : public Stack
{
public:
    Row() : Stack{Axis::Horizontal} {}
};

class Column : public Stack
{
public:
    Column() : Stack{Axis::Vertical} {}
};

/// Children in a grid, filled row by row; each child covers its `span` of cells.
/// Columns are explicit lengths, or responsive: as many as fit at `minColumnWidth` (reflowing on
/// resize). Rows are explicit lengths, a fixed height each, or (default) an equal share each.
class Grid : public Container
{
public:
    explicit Grid(int columns = 2);

    Grid& setColumns(int count);
    Grid& setColumns(std::vector<Length> columns);
    /// Responsive columns: as many as fit at `minWidth`, at least 1, at most `maxColumns` (0 = no cap).
    Grid& setAutoColumns(float minWidth, int maxColumns = 0);
    Grid& setRows(std::vector<Length> rows);
    /// Every row `height` px (0 = rows share the height equally, the default).
    Grid& setRowHeight(float height);
    Grid& setPadding(float padding) { m_padding = padding; invalidate(); return *this; }
    Grid& setGap(float gap)         { m_gap = gap; invalidate(); return *this; }

    /// Columns in use after the last layout.
    [[nodiscard]] int columnCount() const { return m_usedColumns; }

protected:
    void arrange(UiContext const& ctx) override;

private:
    std::vector<Length> m_columns;
    std::vector<Length> m_rows;
    float m_minColumnWidth = 0.0f;
    int   m_maxColumns     = 0;
    float m_rowHeight      = 0.0f;
    float m_padding        = 0.0f;
    float m_gap            = -1.0f;
    int   m_usedColumns    = 0;
};

} // namespace sml
