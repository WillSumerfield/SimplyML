#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "py_app.hpp"
#include "simplyml/ui/bar_chart.hpp"
#include "simplyml/ui/line_chart.hpp"
#include "simplyml/ui/phase_plot.hpp"
#include "simplyml/ui/stats.hpp"

using namespace nb::literals;

namespace
{

// Handles: a widget in an App's tree. They keep the App alive; the widget lives as long as its
// parent keeps it (`clear()` invalidates handles to removed widgets).
struct WidgetRef
{
    nb::object   owner; // the Python App
    PyApp*       app;
    sml::Widget* w;
};
struct ContainerRef : WidgetRef {};
struct StatCardRef : WidgetRef {};
struct ValueRef : WidgetRef {};

nb::object wrap(WidgetRef const& base, sml::Widget* w)
{
    WidgetRef r{base.owner, base.app, w};
    if (dynamic_cast<sml::Container*>(w)) {
        return nb::cast(ContainerRef{r});
    }
    if (dynamic_cast<sml::StatCard*>(w)) {
        return nb::cast(StatCardRef{r});
    }
    if (dynamic_cast<sml::ValueWidget*>(w)) {
        return nb::cast(ValueRef{r});
    }
    return nb::cast(r);
}

template<typename T>
T& as(WidgetRef const& r, char const* what)
{
    auto* p = dynamic_cast<T*>(r.w);
    if (!p) {
        throw nb::type_error((std::string("widget '") + r.w->id() + "' is not a " + what).c_str());
    }
    return *p;
}

/// Layout keywords shared by every widget-creating method.
void applyLayout(sml::Widget& w, nb::kwargs const& kw)
{
    bool const hasSize   = kw.contains("size");
    bool const hasWeight = kw.contains("weight");
    if (hasSize || hasWeight) { // size alone = fixed px; weight alone = share; both = px + share
        w.setExtent({hasSize ? nb::cast<float>(kw["size"]) : 0.0f, hasWeight ? nb::cast<float>(kw["weight"]) : 0.0f});
    }
    for (auto [k, v] : kw) {
        std::string const key = nb::cast<std::string>(k);
        if (key == "size" || key == "weight") {
            continue;
        }
        if (key == "id") {
            w.setId(nb::cast<std::string>(v));
        } else if (key == "fit") {
            if (nb::cast<bool>(v)) {
                w.setExtent(sml::fit());
            }
        } else if (key == "span") {
            if (nb::isinstance<nb::int_>(v)) {
                w.setSpan(nb::cast<int>(v));
            } else {
                auto const s = nb::cast<std::pair<int, int>>(v);
                w.setSpan(s.first, s.second);
            }
        } else if (key == "visible") {
            w.setVisible(nb::cast<bool>(v));
        } else {
            throw nb::type_error(("unexpected keyword argument '" + key + "'").c_str());
        }
    }
}

template<typename T, typename... Args>
nb::object addTo(ContainerRef const& parent, nb::kwargs const& kw, Args&&... args)
{
    UiLock lock{*parent.app};
    auto&  c = as<sml::Container>(parent, "container");
    T&     w = c.add<T>(std::forward<Args>(args)...);
    try {
        applyLayout(w, kw);
    } catch (...) {
        c.remove(w);
        throw;
    }
    return wrap(parent, &w);
}

std::vector<std::string> seriesList(nb::handle h)
{
    if (h.is_none()) {
        return {};
    }
    if (nb::isinstance<nb::str>(h)) {
        return {nb::cast<std::string>(h)};
    }
    return nb::cast<std::vector<std::string>>(h);
}

sml::Theme const& themeOf(WidgetRef const& r) { return r.app->ui.theme(); }

} // namespace

sf::Color toColor(nb::handle h, sml::Theme const& theme)
{
    if (h.is_none()) {
        return sf::Color::Transparent;
    }
    if (nb::isinstance<nb::str>(h)) {
        std::string const n = nb::cast<std::string>(h);
        auto const&       p = theme.palette;
        if (n == "accent" || n == "red") return p.accent;
        if (n == "orange") return p.orange;
        if (n == "yellow") return p.yellow;
        if (n == "green") return p.green;
        if (n == "teal") return p.teal;
        if (n == "blue") return p.blue;
        if (n == "grey" || n == "gray") return p.grey;
        if (n == "white") return p.text;
        throw nb::value_error(("unknown color '" + n + "' (accent, orange, yellow, green, teal, blue, grey, white)").c_str());
    }
    auto const v = nb::cast<std::vector<int>>(h);
    if (v.size() != 3 && v.size() != 4) {
        throw nb::value_error("color must be a name or (r, g, b[, a])");
    }
    auto c8 = [](int x) { return static_cast<std::uint8_t>(std::clamp(x, 0, 255)); };
    return {c8(v[0]), c8(v[1]), c8(v[2]), v.size() == 4 ? c8(v[3]) : std::uint8_t{255}};
}

sml::ValueFormat toFormat(std::string const& spec)
try {
    if (spec == "duration") {
        return sml::ValueFormat::duration();
    }
    auto bad = [&] { return nb::value_error(("bad format '" + spec + "' (e.g. '.3', '04d', '.1%', 'duration')").c_str()); };
    if (!spec.empty() && spec.back() == '%') {
        std::string const body = spec.substr(0, spec.size() - 1);
        if (body.empty()) {
            return sml::ValueFormat::percent(0);
        }
        if (body[0] != '.') {
            throw bad();
        }
        return sml::ValueFormat::percent(std::stoi(body.substr(1)));
    }
    if (!spec.empty() && spec.back() == 'd') {
        std::string const body = spec.substr(0, spec.size() - 1);
        return sml::ValueFormat::integer(body.empty() ? 0 : std::stoi(body));
    }
    std::string body = spec;
    if (!body.empty() && body.back() == 'f') {
        body.pop_back();
    }
    if (body.size() < 2 || body[0] != '.') {
        throw bad();
    }
    return sml::ValueFormat::number(std::stoi(body.substr(1)));
} catch (std::logic_error const&) { // stoi
    throw nb::value_error(("bad format '" + spec + "' (e.g. '.3', '04d', '.1%', 'duration')").c_str());
}

void bindUi(nb::module_& m)
{
    nb::class_<WidgetRef>(m, "Widget", "A widget in an App's layout.")
        .def_prop_rw("id",
            [](WidgetRef const& r) { UiLock l{*r.app}; return r.w->id(); },
            [](WidgetRef const& r, std::string id) { UiLock l{*r.app}; r.w->setId(std::move(id)); })
        .def_prop_rw("visible",
            [](WidgetRef const& r) { UiLock l{*r.app}; return r.w->visible(); },
            [](WidgetRef const& r, bool v) { UiLock l{*r.app}; r.w->setVisible(v); })
        .def_prop_ro("bounds", [](WidgetRef const& r) {
            UiLock l{*r.app};
            auto const b = r.w->bounds();
            return std::make_tuple(b.position.x, b.position.y, b.size.x, b.size.y);
        }, "(x, y, width, height) in window pixels, after the last layout.")
        .def_prop_rw("title",
            [](WidgetRef const& r) { UiLock l{*r.app}; return as<sml::Panel>(r, "panel").title(); },
            [](WidgetRef const& r, std::string t) { UiLock l{*r.app}; as<sml::Panel>(r, "panel").setTitle(std::move(t)); },
            "Panel title (panels, charts, cards).")
        .def("layout", [](WidgetRef const& r, nb::kwargs kw) { UiLock l{*r.app}; applyLayout(*r.w, kw); },
            "Changes layout keywords: id, size (px), weight (share), fit, span, visible.")
        .def("__repr__", [](WidgetRef const& r) { return "<simplyml.Widget id='" + r.w->id() + "'>"; });

    nb::class_<ValueRef, WidgetRef>(m, "ValueWidget", "A widget showing one value (stat, gauge, status).")
        .def("set_value", [](ValueRef const& r, double v) { UiLock l{*r.app}; as<sml::ValueWidget>(r, "value widget").setValue(v); },
            "v"_a, "Shows `v` when no series is bound (or it has no data yet).")
        .def("set_text", [](ValueRef const& r, std::string t) { UiLock l{*r.app}; as<sml::ValueWidget>(r, "value widget").setText(std::move(t)); },
            "text"_a, "Shows fixed text instead of a number.")
        .def("set_series", [](ValueRef const& r, std::string s) { UiLock l{*r.app}; as<sml::ValueWidget>(r, "value widget").setSeries(std::move(s)); },
            "series"_a);

    nb::class_<StatCardRef, WidgetRef>(m, "StatCard", "Panel of stat rows.")
        .def("value", [](StatCardRef const& r, std::string label, std::string series, std::string fmt) {
            UiLock l{*r.app};
            return wrap(r, &as<sml::StatCard>(r, "stat card").addValue(std::move(label), std::move(series), toFormat(fmt)));
        }, "label"_a, "series"_a, "fmt"_a = ".2", "Small label over the series' latest value.")
        .def("big", [](StatCardRef const& r, std::string label, std::string series, std::string fmt) {
            UiLock l{*r.app};
            return wrap(r, &as<sml::StatCard>(r, "stat card").addBig(std::move(label), std::move(series), toFormat(fmt)));
        }, "label"_a, "series"_a, "fmt"_a = "d", "Large headline value.")
        .def("gauge", [](StatCardRef const& r, std::string label, std::string series, double lo, double hi,
                         nb::handle color, std::string fmt) {
            auto const valueFmt = toFormat(fmt);
            sf::Color const c   = toColor(color, themeOf(r));
            UiLock l{*r.app};
            auto& g = as<sml::StatCard>(r, "stat card").addGauge(std::move(label), std::move(series), lo, hi, c);
            g.setFormat(valueFmt);
            return wrap(r, &g);
        }, "label"_a, "series"_a, "lo"_a = 0.0, "hi"_a = 1.0, "color"_a = nb::none(), "fmt"_a = ".2",
            "Progress bar for a value in [lo, hi].")
        .def("status", [](StatCardRef const& r, std::string label, std::string series, std::string on, std::string off) {
            UiLock l{*r.app};
            return wrap(r, &as<sml::StatCard>(r, "stat card").addStatus(std::move(label), std::move(series), std::move(on), std::move(off)));
        }, "label"_a, "series"_a, "on"_a = "Enabled", "off"_a = "Disabled",
            "Dot + text: on while the series' latest value is > 0.5.");

    nb::class_<ContainerRef, WidgetRef>(m, "Container",
        "Row, Column or Grid. Its methods add a child and return it; every one takes the layout "
        "keywords id, size (px), weight (share of the rest), fit, span=(columns, rows) and visible.")
        .def("row", [](ContainerRef const& r, std::optional<float> padding, std::optional<float> gap, nb::kwargs kw) {
            auto o = addTo<sml::Row>(r, kw);
            UiLock l{*r.app};
            auto& s = as<sml::Stack>(nb::cast<WidgetRef const&>(o), "row");
            if (padding) s.setPadding(*padding);
            if (gap) s.setGap(*gap);
            return o;
        }, "padding"_a = nb::none(), "gap"_a = nb::none(), "kw"_a, "Children side by side.")
        .def("column", [](ContainerRef const& r, std::optional<float> padding, std::optional<float> gap, nb::kwargs kw) {
            auto o = addTo<sml::Column>(r, kw);
            UiLock l{*r.app};
            auto& s = as<sml::Stack>(nb::cast<WidgetRef const&>(o), "column");
            if (padding) s.setPadding(*padding);
            if (gap) s.setGap(*gap);
            return o;
        }, "padding"_a = nb::none(), "gap"_a = nb::none(), "kw"_a, "Children stacked top to bottom.")
        .def("grid", [](ContainerRef const& r, int columns, std::optional<float> minColumnWidth, int maxColumns,
                        float rowHeight, std::optional<float> gap, nb::kwargs kw) {
            auto o = addTo<sml::Grid>(r, kw, columns);
            UiLock l{*r.app};
            auto& g = as<sml::Grid>(nb::cast<WidgetRef const&>(o), "grid");
            if (minColumnWidth) g.setAutoColumns(*minColumnWidth, maxColumns);
            if (rowHeight > 0) g.setRowHeight(rowHeight);
            if (gap) g.setGap(*gap);
            return o;
        }, "columns"_a = 2, "min_column_width"_a = nb::none(), "max_columns"_a = 0, "row_height"_a = 0.0f,
            "gap"_a = nb::none(), "kw"_a,
            "Cells filled row by row. With `min_column_width`, the column count follows the window width.")
        .def("panel", [](ContainerRef const& r, std::string title, nb::handle color, nb::kwargs kw) {
            return addTo<sml::Panel>(r, kw, std::move(title), toColor(color, themeOf(r)));
        }, "title"_a = "", "color"_a = nb::none(), "kw"_a, "Empty card.")
        .def("line_chart", [](ContainerRef const& r, std::string title, nb::handle series, nb::handle color,
                              nb::handle colors, nb::handle labels, std::size_t window, bool area,
                              std::optional<std::pair<double, double>> yRange, bool includeZero, std::string fmt,
                              std::optional<std::string> yFmt, std::string xFmt, nb::kwargs kw) {
            auto const names = seriesList(series);
            auto const cols  = colors.is_none() ? std::vector<nb::handle>{} : nb::cast<std::vector<nb::handle>>(colors);
            auto const labs  = labels.is_none() ? std::vector<std::string>{} : nb::cast<std::vector<std::string>>(labels);
            std::vector<sf::Color> lineColors;
            for (auto h : cols) {
                lineColors.push_back(toColor(h, themeOf(r)));
            }
            auto const valueFmt = toFormat(fmt);
            auto const xFormat  = toFormat(xFmt);
            auto const yFormat  = yFmt ? std::optional{toFormat(*yFmt)} : std::nullopt;
            auto o = addTo<sml::LineChart>(r, kw, std::move(title), std::string{}, toColor(color, themeOf(r)));
            UiLock l{*r.app};
            auto& c = as<sml::LineChart>(nb::cast<WidgetRef const&>(o), "line chart");
            for (std::size_t i = 0; i < names.size(); ++i) {
                c.addSeries(names[i], i < lineColors.size() ? lineColors[i] : sf::Color::Transparent,
                            i < labs.size() ? labs[i] : std::string{});
            }
            c.setWindow(window).setArea(area).setValueFormat(valueFmt).setXFormat(xFormat);
            if (yRange) c.setYRange(yRange->first, yRange->second); else c.setAutoY(includeZero);
            if (yFormat) c.setYFormat(*yFormat);
            return o;
        }, "title"_a = "", "series"_a = nb::none(), "color"_a = nb::none(), "colors"_a = nb::none(),
            "labels"_a = nb::none(), "window"_a = 0, "area"_a = true, "y_range"_a = nb::none(),
            "include_zero"_a = true, "fmt"_a = ".4", "y_fmt"_a = nb::none(), "x_fmt"_a = "d", "kw"_a,
            "Line chart of one series (str) or several (list; legend shown). `window` = newest N points (0 = all).")
        .def("bar_chart", [](ContainerRef const& r, std::string title, std::string series, nb::handle color,
                             std::size_t window, std::optional<std::pair<double, double>> yRange, std::string fmt,
                             nb::kwargs kw) {
            auto const valueFmt = toFormat(fmt);
            auto o = addTo<sml::BarChart>(r, kw, std::move(title), std::move(series), toColor(color, themeOf(r)));
            UiLock l{*r.app};
            auto& c = as<sml::BarChart>(nb::cast<WidgetRef const&>(o), "bar chart");
            c.setWindow(window).setValueFormat(valueFmt);
            if (yRange) c.setYRange(yRange->first, yRange->second);
            return o;
        }, "title"_a = "", "series"_a = "", "color"_a = nb::none(), "window"_a = 50, "y_range"_a = nb::none(),
            "fmt"_a = ".4", "kw"_a, "Bars for the newest `window` values of a series.")
        .def("phase_plot", [](ContainerRef const& r, std::string title, std::string x, std::string y, nb::handle color,
                              std::size_t maxPoints, std::optional<std::tuple<double, double, double, double>> ranges,
                              nb::kwargs kw) {
            auto o = addTo<sml::PhasePlot>(r, kw, std::move(title), std::move(x), std::move(y), toColor(color, themeOf(r)));
            UiLock l{*r.app};
            auto& p = as<sml::PhasePlot>(nb::cast<WidgetRef const&>(o), "phase plot");
            p.setMaxPoints(maxPoints);
            if (ranges) p.setRanges(std::get<0>(*ranges), std::get<1>(*ranges), std::get<2>(*ranges), std::get<3>(*ranges));
            return o;
        }, "title"_a = "", "x"_a = "", "y"_a = "", "color"_a = nb::none(), "max_points"_a = 500,
            "ranges"_a = nb::none(), "kw"_a, "XY trajectory of two series paired by index; `ranges` = (xlo, xhi, ylo, yhi).")
        .def("stat_tile", [](ContainerRef const& r, std::string label, std::string series, nb::handle color,
                             std::string fmt, nb::kwargs kw) {
            if (!kw.contains("size") && !kw.contains("weight")) {
                kw["fit"] = true;
            }
            return addTo<sml::StatTile>(r, kw, std::move(label), std::move(series), toFormat(fmt), toColor(color, themeOf(r)));
        }, "label"_a = "", "series"_a = "", "color"_a = nb::none(), "fmt"_a = "d", "kw"_a,
            "Compact panel: label and large value (fits its height in columns by default).")
        .def("stat_card", [](ContainerRef const& r, std::string title, nb::handle color, nb::kwargs kw) {
            return addTo<sml::StatCard>(r, kw, std::move(title), toColor(color, themeOf(r)));
        }, "title"_a = "", "color"_a = nb::none(), "kw"_a, "Panel of stat rows; add rows with its methods.")
        .def("gauge", [](ContainerRef const& r, std::string label, std::string series, double lo, double hi,
                         nb::handle color, std::string fmt, nb::kwargs kw) {
            if (!kw.contains("size") && !kw.contains("weight")) {
                kw["fit"] = true;
            }
            auto const valueFmt = toFormat(fmt);
            auto o = addTo<sml::Gauge>(r, kw, std::move(label), std::move(series), lo, hi, toColor(color, themeOf(r)));
            UiLock l{*r.app};
            as<sml::Gauge>(nb::cast<WidgetRef const&>(o), "gauge").setFormat(valueFmt);
            return o;
        }, "label"_a = "", "series"_a = "", "lo"_a = 0.0, "hi"_a = 1.0, "color"_a = nb::none(), "fmt"_a = ".2",
            "kw"_a, "Labelled progress bar.")
        .def("clear", [](ContainerRef const& r) { UiLock l{*r.app}; as<sml::Container>(r, "container").clear(); },
            "Removes every child (their handles become invalid).")
        .def("__len__", [](ContainerRef const& r) { UiLock l{*r.app}; return as<sml::Container>(r, "container").children().size(); });

    // Exposed for App: root container + lookup.
    m.def("_ui_root", [](nb::object owner) {
        PyApp& a = nb::cast<PyApp&>(owner);
        UiLock l{a};
        return wrap(WidgetRef{owner, &a, nullptr}, &a.ui.root());
    });
    m.def("_ui_find", [](nb::object owner, std::string_view id) -> nb::object {
        PyApp& a = nb::cast<PyApp&>(owner);
        UiLock l{a};
        sml::Widget* w = a.ui.find(id);
        if (!w) {
            throw nb::key_error(("no widget with id '" + std::string(id) + "'").c_str());
        }
        return wrap(WidgetRef{owner, &a, nullptr}, w);
    });
}
