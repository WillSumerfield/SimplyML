#pragma once
// Python handles to widgets in an App's tree, and the helpers that create them.
#include <cmath>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "py_app.hpp"
#include "simplyml/ml/cart_pendulum_view.hpp"
#include "simplyml/ml/network_view.hpp"
#include "simplyml/ui/controls.hpp"
#include "simplyml/ui/stats.hpp"

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
struct NetworkViewRef : WidgetRef {};
struct CartPendulumRef : WidgetRef {};
/// A control. Its value is read from the Control Store (no Ui lock), typed by `kind`.
struct ControlRef : WidgetRef
{
    enum class Kind { Number, Integer, Bool, Count, Option };
    Kind                     kind = Kind::Number;
    std::string              name;
    std::vector<std::string> options; // Option
};

/// Where children of this handle go: the container itself, or a control panel's rows.
inline sml::Container* containerOf(sml::Widget* w)
{
    if (auto* c = dynamic_cast<sml::Container*>(w)) {
        return c;
    }
    if (auto* p = dynamic_cast<sml::ControlPanel*>(w)) {
        return &p->rows();
    }
    return nullptr;
}

inline ControlRef controlRef(WidgetRef const& r, sml::Control& c)
{
    using K = ControlRef::Kind;
    ControlRef out{r};
    out.name = c.id();
    if (dynamic_cast<sml::Toggle*>(&c)) {
        out.kind = K::Bool;
    } else if (dynamic_cast<sml::Button*>(&c)) {
        out.kind = K::Count;
    } else if (auto* s = dynamic_cast<sml::Select*>(&c)) {
        out.kind    = K::Option;
        out.options = s->options();
    } else if (auto* n = dynamic_cast<sml::NumberField*>(&c); n && n->integer()) {
        out.kind = K::Integer;
    } else if (auto* sl = dynamic_cast<sml::Slider*>(&c); sl && sl->step() >= 1.0 && sl->step() == std::floor(sl->step())
                                                         && sl->lo() == std::floor(sl->lo())) {
        out.kind = K::Integer;
    }
    return out;
}

inline nb::object wrap(WidgetRef const& base, sml::Widget* w)
{
    WidgetRef r{base.owner, base.app, w};
    if (containerOf(w)) {
        return nb::cast(ContainerRef{r});
    }
    if (dynamic_cast<sml::StatCard*>(w)) {
        return nb::cast(StatCardRef{r});
    }
    if (dynamic_cast<sml::NetworkView*>(w)) {
        return nb::cast(NetworkViewRef{r});
    }
    if (dynamic_cast<sml::CartPendulumView*>(w)) {
        return nb::cast(CartPendulumRef{r});
    }
    if (dynamic_cast<sml::ValueWidget*>(w)) {
        return nb::cast(ValueRef{r});
    }
    if (auto* c = dynamic_cast<sml::Control*>(w)) {
        return nb::cast(controlRef(r, *c));
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
inline void applyLayout(sml::Widget& w, nb::kwargs const& kw)
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
    auto*  cp = containerOf(parent.w);
    if (!cp) {
        throw nb::type_error(("widget '" + parent.w->id() + "' holds no children").c_str());
    }
    auto&  c = *cp;
    T&     w = c.add<T>(std::forward<Args>(args)...);
    try {
        applyLayout(w, kw);
    } catch (...) {
        c.remove(w);
        throw;
    }
    return wrap(parent, &w);
}

inline std::vector<std::string> seriesList(nb::handle h)
{
    if (h.is_none()) {
        return {};
    }
    if (nb::isinstance<nb::str>(h)) {
        return {nb::cast<std::string>(h)};
    }
    return nb::cast<std::vector<std::string>>(h);
}

inline sml::Theme const& themeOf(WidgetRef const& r) { return r.app->ui.theme(); }


/// Control classes, control methods on `container`, `Controls` and the key-binding helpers.
void bindControls(nb::module_& m, nb::class_<ContainerRef, WidgetRef>& container);
/// ML widget classes and their methods on `container`.
void bindMl(nb::module_& m, nb::class_<ContainerRef, WidgetRef>& container);
