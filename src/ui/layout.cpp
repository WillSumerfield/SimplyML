#include "simplyml/ui/layout.hpp"

#include <algorithm>
#include <cmath>

namespace sml
{

std::vector<Span1D> distribute(std::vector<Length> const& lengths, float total, float gap, Align align)
{
    std::vector<Span1D> out(lengths.size());
    if (lengths.empty()) {
        return out;
    }
    float fixed = 0.0f;
    float frs   = 0.0f;
    for (auto const& l : lengths) {
        fixed += std::max(l.px, 0.0f);
        frs += std::max(l.fr, 0.0f);
    }
    float const gaps = gap * static_cast<float>(lengths.size() - 1);
    float const free = std::max(0.0f, total - fixed - gaps);

    float pos = 0.0f;
    if (frs <= 0.0f) {
        pos = align == Align::Center ? 0.5f * free : align == Align::End ? free : 0.0f;
    }
    for (std::size_t i = 0; i < lengths.size(); ++i) {
        float const share = frs > 0.0f ? free * std::max(lengths[i].fr, 0.0f) / frs : 0.0f;
        out[i] = {pos, std::max(lengths[i].px, 0.0f) + share};
        pos += out[i].size + gap;
    }
    return out;
}

void Stack::arrange(UiContext const& ctx)
{
    float const pad = m_padding < 0.0f ? ctx.theme.px(ctx.theme.margin) : m_padding;
    float const gap = m_gap < 0.0f ? ctx.theme.px(ctx.theme.gap) : m_gap;
    sf::FloatRect const b = bounds();
    sf::FloatRect const inner{b.position + sf::Vector2f{pad, pad},
                              {std::max(0.0f, b.size.x - 2 * pad), std::max(0.0f, b.size.y - 2 * pad)}};
    bool const horizontal = m_axis == Axis::Horizontal;
    std::vector<Widget*> shown;
    std::vector<Length>  lengths;
    for (auto& c : m_children) {
        if (c->visible()) {
            shown.push_back(c.get());
            Length l = c->extent();
            if (l.px <= 0.0f && l.fr <= 0.0f) {
                sf::Vector2f const n = c->naturalSize(ctx);
                l.px = horizontal ? n.x : n.y;
            }
            lengths.push_back(l);
        }
    }
    auto const spans = distribute(lengths, horizontal ? inner.size.x : inner.size.y, gap, m_align);
    for (std::size_t i = 0; i < shown.size(); ++i) {
        sf::FloatRect r = inner;
        if (horizontal) {
            r.position.x += spans[i].start;
            r.size.x = spans[i].size;
        } else {
            r.position.y += spans[i].start;
            r.size.y = spans[i].size;
        }
        shown[i]->layout(r, ctx);
    }
}

Grid::Grid(int columns)
{
    setColumns(columns);
}

Grid& Grid::setColumns(int count)
{
    m_columns.assign(static_cast<std::size_t>(std::max(count, 1)), fr(1.0f));
    m_minColumnWidth = 0.0f;
    invalidate();
    return *this;
}

Grid& Grid::setColumns(std::vector<Length> columns)
{
    m_columns = columns.empty() ? std::vector<Length>{fr(1.0f)} : std::move(columns);
    m_minColumnWidth = 0.0f;
    invalidate();
    return *this;
}

Grid& Grid::setAutoColumns(float minWidth, int maxColumns)
{
    m_minColumnWidth = std::max(minWidth, 1.0f);
    m_maxColumns     = maxColumns;
    invalidate();
    return *this;
}

Grid& Grid::setRows(std::vector<Length> rows)
{
    m_rows = std::move(rows);
    invalidate();
    return *this;
}

Grid& Grid::setRowHeight(float height)
{
    m_rowHeight = height;
    m_rows.clear();
    invalidate();
    return *this;
}

void Grid::arrange(UiContext const& ctx)
{
    float const pad = m_padding < 0.0f ? ctx.theme.px(ctx.theme.margin) : m_padding;
    float const gap = m_gap < 0.0f ? ctx.theme.px(ctx.theme.gap) : m_gap;
    sf::FloatRect const b = bounds();
    sf::FloatRect const inner{b.position + sf::Vector2f{pad, pad},
                              {std::max(0.0f, b.size.x - 2 * pad), std::max(0.0f, b.size.y - 2 * pad)}};

    std::vector<Length> cols = m_columns;
    if (m_minColumnWidth > 0.0f) {
        int n = static_cast<int>(std::floor((inner.size.x + gap) / (m_minColumnWidth + gap)));
        n = std::max(n, 1);
        if (m_maxColumns > 0) {
            n = std::min(n, m_maxColumns);
        }
        cols.assign(static_cast<std::size_t>(n), fr(1.0f));
    }
    int const ncols = static_cast<int>(cols.size());
    m_usedColumns   = ncols;

    // Auto-flow: each child takes the first free spot (row-major) where its span fits.
    struct Cell { Widget* w; int col; int row; int cs; int rs; };
    std::vector<Cell>              cells;
    std::vector<std::vector<bool>> used;
    auto taken = [&](int r, int c) { return r < static_cast<int>(used.size()) && used[r][c]; };
    for (auto& child : m_children) {
        if (!child->visible()) {
            continue;
        }
        int const cs = std::min(child->span().x, ncols);
        int const rs = child->span().y;
        for (int r = 0;; ++r) {
            int found = -1;
            for (int c = 0; c + cs <= ncols && found < 0; ++c) {
                bool ok = true;
                for (int rr = r; rr < r + rs && ok; ++rr) {
                    for (int cc = c; cc < c + cs && ok; ++cc) {
                        ok = !taken(rr, cc);
                    }
                }
                if (ok) {
                    found = c;
                }
            }
            if (found >= 0) {
                if (static_cast<int>(used.size()) < r + rs) {
                    used.resize(static_cast<std::size_t>(r + rs), std::vector<bool>(static_cast<std::size_t>(ncols)));
                }
                for (int rr = r; rr < r + rs; ++rr) {
                    for (int cc = found; cc < found + cs; ++cc) {
                        used[rr][cc] = true;
                    }
                }
                cells.push_back({child.get(), found, r, cs, rs});
                break;
            }
        }
    }

    std::size_t const nrows = used.size();
    std::vector<Length> rows = m_rows;
    Length const fill = m_rowHeight > 0.0f ? px(m_rowHeight) : fr(1.0f);
    if (rows.size() < nrows) {
        rows.resize(nrows, fill);
    }
    auto const cx = distribute(cols, inner.size.x, gap, Align::Start);
    auto const cy = distribute(rows, inner.size.y, gap, Align::Start);
    for (auto const& cell : cells) {
        auto const& c0 = cx[static_cast<std::size_t>(cell.col)];
        auto const& c1 = cx[static_cast<std::size_t>(cell.col + cell.cs - 1)];
        auto const& r0 = cy[static_cast<std::size_t>(cell.row)];
        auto const& r1 = cy[static_cast<std::size_t>(cell.row + cell.rs - 1)];
        cell.w->layout({inner.position + sf::Vector2f{c0.start, r0.start},
                        {c1.start + c1.size - c0.start, r1.start + r1.size - r0.start}},
                       ctx);
    }
}

} // namespace sml
