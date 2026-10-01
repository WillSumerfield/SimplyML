#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/vector.h>

#include "simplyml/core/keys.hpp"
#include "simplyml/ui/key_bindings.hpp"
#include "widget_refs.hpp"

using namespace nb::literals;

namespace
{

using Kind = ControlRef::Kind;

/// A Python object kept by C++ (callbacks). Its last owner may be a UI-thread copy, so it is
/// released under the GIL.
std::shared_ptr<nb::object> hold(nb::handle fn)
{
    return {new nb::object(nb::borrow(fn)), [](nb::object* o) {
                nb::gil_scoped_acquire gil;
                delete o;
            }};
}

/// Runs `fn` on the UI thread with the GIL; errors are printed, never thrown into the UI loop.
template<typename... Args>
void callPython(nb::object const& fn, char const* what, Args&&... args)
{
    nb::gil_scoped_acquire gil;
    try {
        fn(std::forward<Args>(args)...);
    } catch (nb::python_error& e) {
        e.discard_as_unraisable(what);
    } catch (std::exception const&) {
    }
}

nb::object toPython(Kind kind, std::vector<std::string> const& options, double v)
{
    switch (kind) {
        case Kind::Bool:    return nb::bool_(v > 0.5);
        case Kind::Count:
        case Kind::Integer: return nb::int_(static_cast<long long>(std::llround(v)));
        case Kind::Option: {
            auto const i = static_cast<std::size_t>(std::max(0.0, v));
            return i < options.size() ? nb::object(nb::str(options[i].c_str())) : nb::object(nb::none());
        }
        case Kind::Number:  return nb::float_(v);
    }
    return nb::float_(v);
}

double fromPython(ControlRef const& r, nb::handle v)
{
    if (r.kind == Kind::Option && nb::isinstance<nb::str>(v)) {
        std::string const s = nb::cast<std::string>(v);
        for (std::size_t i = 0; i < r.options.size(); ++i) {
            if (r.options[i] == s) {
                return static_cast<double>(i);
            }
        }
        throw nb::value_error(("'" + s + "' is not an option of '" + r.name + "'").c_str());
    }
    return nb::cast<double>(v);
}

sf::Keyboard::Key toKey(std::string const& name)
{
    auto const k = sml::keyFromName(name);
    if (!k) {
        throw nb::value_error(("unknown key '" + name + "' (see simplyml.key_names())").c_str());
    }
    return *k;
}

/// Creates control `T` in `parent`, runs `configure` on it, declares its value in the Control Store
/// (an existing value wins) and wires the optional key and `on_change` callback. Defaults to its
/// natural height.
template<typename T, typename Configure, typename... Args>
nb::object addControl(ContainerRef const& parent, nb::kwargs kw, std::optional<std::string> const& key,
                      nb::handle onChange, Configure&& configure, Args&&... args)
{
    std::optional<sf::Keyboard::Key> const k = key ? std::optional{toKey(*key)} : std::nullopt;
    if (!onChange.is_none() && !PyCallable_Check(onChange.ptr())) {
        throw nb::type_error("on_change must be callable");
    }
    if (!kw.contains("size") && !kw.contains("weight")) {
        kw["fit"] = true;
    }
    nb::object o = addTo<T>(parent, kw, std::forward<Args>(args)...);
    UiLock     l{*parent.app};
    auto&      c = static_cast<T&>(*nb::cast<WidgetRef const&>(o).w);
    configure(c);
    c.setValue(c.value()); // re-sanitize with the configured range/step
    parent.app->app.controls().declare(c.id(), c.value());
    o = wrap(parent, &c); // the handle's value type depends on the configuration
    auto const& ref = nb::cast<ControlRef const&>(o);
    if (k) {
        parent.app->ui.bindKey(*k, c);
    }
    if (!onChange.is_none()) {
        auto fn = hold(onChange);
        c.onChange([fn, kind = ref.kind, options = ref.options](double v) {
            nb::gil_scoped_acquire gil; // toPython builds Python objects before callPython runs
            callPython(*fn, "simplyml on_change callback", toPython(kind, options, v));
        });
    }
    return o;
}

auto const noConfig = [](auto&) {};

std::string labelOr(std::optional<std::string> const& label, std::string const& name)
{
    return label ? *label : name;
}

} // namespace

void bindControls(nb::module_& m, nb::class_<ContainerRef, WidgetRef>& container)
{
    nb::class_<ControlRef, WidgetRef>(m, "Control",
        "An interactive widget editing one named value. Reading `value` costs one short lock on the "
        "Control Store, so it is fine every step of a training loop.")
        .def_prop_ro("name", [](ControlRef const& r) { return r.name; }, "Key in `app.controls` (also the id).")
        .def_prop_rw("value",
            [](ControlRef const& r) {
                return toPython(r.kind, r.options, r.app->app.controls().get(r.name, 0.0));
            },
            [](ControlRef const& r, nb::handle v) { r.app->app.controls().set(r.name, fromPython(r, v)); },
            "float (slider, number), int (integer slider/number, button clicks), bool (toggle) or the "
            "option str (select). Setting it moves the widget on the next frame.")
        .def_prop_ro("index", [](ControlRef const& r) {
            return static_cast<int>(std::llround(r.app->app.controls().get(r.name, 0.0)));
        }, "Select: index of the chosen option.")
        .def_prop_ro("options", [](ControlRef const& r) { return r.options; }, "Select: its options.")
        .def_prop_rw("label",
            [](ControlRef const& r) { UiLock l{*r.app}; return as<sml::Control>(r, "control").label(); },
            [](ControlRef const& r, std::string s) { UiLock l{*r.app}; as<sml::Control>(r, "control").setLabel(std::move(s)); })
        .def_prop_rw("enabled",
            [](ControlRef const& r) { UiLock l{*r.app}; return as<sml::Control>(r, "control").enabled(); },
            [](ControlRef const& r, bool e) { UiLock l{*r.app}; as<sml::Control>(r, "control").setEnabled(e); },
            "Disabled controls ignore input and draw faded.")
        .def("__repr__", [](ControlRef const& r) { return "<simplyml.Control name='" + r.name + "'>"; });

    container
        .def("button", [](ContainerRef const& r, std::string name, std::optional<std::string> label, nb::handle color,
                          std::optional<std::string> key, nb::handle onClick, nb::kwargs kw) {
            sf::Color const c = toColor(color, themeOf(r));
            return addControl<sml::Button>(r, kw, key, onClick, noConfig, name, labelOr(label, name), c);
        }, "name"_a, "label"_a = nb::none(), "color"_a = nb::none(), "key"_a = nb::none(), "on_click"_a = nb::none(),
            "kw"_a,
            "Push button; its value counts clicks. Each click also queues Event(custom, name=name). "
            "`on_click(clicks)` runs on the UI thread and must be quick.")
        .def("toggle", [](ContainerRef const& r, std::string name, std::optional<std::string> label, bool on,
                          std::optional<std::string> key, nb::handle onChange, nb::kwargs kw) {
            return addControl<sml::Toggle>(r, kw, key, onChange, noConfig, name, labelOr(label, name), on);
        }, "name"_a, "label"_a = nb::none(), "on"_a = false, "key"_a = nb::none(), "on_change"_a = nb::none(), "kw"_a,
            "On/off switch (value: bool). `key` flips it.")
        .def("slider", [](ContainerRef const& r, std::string name, std::optional<std::string> label, double lo, double hi,
                          std::optional<double> value, bool log, double step, std::optional<std::string> fmt,
                          nb::handle color, nb::handle onChange, nb::kwargs kw) {
            if (log && !(lo > 0.0 && hi > lo)) {
                throw nb::value_error("log slider needs 0 < lo < hi");
            }
            auto const      f = fmt ? std::optional{toFormat(*fmt)} : std::nullopt;
            sf::Color const c = toColor(color, themeOf(r));
            return addControl<sml::Slider>(r, kw, std::nullopt, onChange, [&](sml::Slider& s) {
                s.setLog(log).setStep(step).setColor(c);
                if (f) {
                    s.setFormat(*f);
                }
            }, name, labelOr(label, name), lo, hi, value.value_or(lo));
        }, "name"_a, "label"_a = nb::none(), "lo"_a = 0.0, "hi"_a = 1.0, "value"_a = nb::none(), "log"_a = false,
            "step"_a = 0.0, "fmt"_a = nb::none(), "color"_a = nb::none(), "on_change"_a = nb::none(), "kw"_a,
            "Draggable value in [lo, hi] (default value: lo). `log=True` for learning rates; `step=1` for integers.")
        .def("select", [](ContainerRef const& r, std::string name, std::vector<std::string> options,
                          std::optional<std::string> label, nb::handle value, bool radio, nb::handle color,
                          std::optional<std::string> key, nb::handle onChange, nb::kwargs kw) {
            if (options.empty()) {
                throw nb::value_error("select needs at least one option");
            }
            ControlRef probe;
            probe.kind    = Kind::Option;
            probe.name    = name;
            probe.options = options;
            int const       index = value.is_none() ? 0 : static_cast<int>(fromPython(probe, value));
            sf::Color const c     = toColor(color, themeOf(r));
            return addControl<sml::Select>(r, kw, key, onChange, [&](sml::Select& s) { s.setColor(c); }, name,
                                           labelOr(label, name), options, index,
                                           radio ? sml::Select::Style::Radio : sml::Select::Style::Segmented);
        }, "name"_a, "options"_a, "label"_a = nb::none(), "value"_a = nb::none(), "radio"_a = false,
            "color"_a = nb::none(), "key"_a = nb::none(), "on_change"_a = nb::none(), "kw"_a,
            "One of `options` (value: the option str; `value` = initial str or index). Segmented row, or a "
            "radio list with `radio=True`. `key` picks the next option.")
        .def("number", [](ContainerRef const& r, std::string name, std::optional<std::string> label, double value,
                          std::optional<double> lo, std::optional<double> hi, bool integer,
                          std::optional<std::string> fmt, nb::handle onChange, nb::kwargs kw) {
            auto const f = fmt ? std::optional{toFormat(*fmt)} : std::nullopt;
            double const inf = std::numeric_limits<double>::infinity();
            return addControl<sml::NumberField>(r, kw, std::nullopt, onChange, [&](sml::NumberField& n) {
                n.setInteger(integer);
                if (f) {
                    n.setFormat(*f);
                }
            }, name, labelOr(label, name), value, lo.value_or(-inf), hi.value_or(inf));
        }, "name"_a, "label"_a = nb::none(), "value"_a = 0.0, "lo"_a = nb::none(), "hi"_a = nb::none(),
            "integer"_a = false, "fmt"_a = nb::none(), "on_change"_a = nb::none(), "kw"_a,
            "Typed number; applied on Enter or clicking away, rejected (red flash) outside [lo, hi].")
        .def("control_panel", [](ContainerRef const& r, std::string title, nb::handle color, nb::kwargs kw) {
            if (!kw.contains("size") && !kw.contains("weight")) {
                kw["fit"] = true;
            }
            return addTo<sml::ControlPanel>(r, kw, std::move(title), toColor(color, themeOf(r)));
        }, "title"_a = "", "color"_a = nb::none(), "kw"_a,
            "Panel stacking controls; add them with its methods (fits its height by default).")
        .def("key_bindings", [](ContainerRef const& r, std::string title, nb::handle color, nb::kwargs kw) {
            if (!kw.contains("size") && !kw.contains("weight")) {
                kw["fit"] = true;
            }
            return addTo<sml::KeyBindings>(r, kw, std::move(title), toColor(color, themeOf(r)));
        }, "title"_a = "Keys", "color"_a = nb::none(), "kw"_a, "Panel listing every key binding.");

    nb::class_<sml::ControlStore>(m, "Controls",
        "Named control values (`app.controls`): what sliders, toggles and buttons hold, as floats. "
        "Thread-safe; writes show up in the widgets on the next frame.")
        .def("__getitem__", [](sml::ControlStore const& c, std::string_view name) {
            auto const v = c.get(name);
            if (!v) {
                throw nb::key_error(std::string(name).c_str());
            }
            return *v;
        })
        .def("__setitem__", [](sml::ControlStore& c, std::string_view name, double v) { c.set(name, v); })
        .def("__contains__", [](sml::ControlStore const& c, std::string_view name) { return c.contains(name); })
        .def("get", [](sml::ControlStore const& c, std::string_view name, std::optional<double> fallback) {
            auto const v = c.get(name);
            return v ? v : fallback;
        }, "name"_a, "default"_a = nb::none())
        .def("to_dict", [](sml::ControlStore const& c) {
            nb::dict d;
            for (auto const& [k, v] : c.all()) {
                d[k.c_str()] = v;
            }
            return d;
        });

    m.def("_bind_key", [](nb::object owner, std::string const& key, std::string description, nb::handle callback) {
        PyApp&     a = nb::cast<PyApp&>(owner);
        auto const k = toKey(key);
        std::function<void()> action;
        if (!callback.is_none()) {
            auto fn = hold(callback);
            action  = [fn] { callPython(*fn, "simplyml key callback"); };
        }
        UiLock l{a};
        a.ui.bindKey(k, std::move(description), std::move(action));
    }, "owner"_a, "key"_a, "description"_a, "callback"_a.none());
    m.def("_unbind_key", [](nb::object owner, std::string const& key) {
        PyApp& a = nb::cast<PyApp&>(owner);
        UiLock l{a};
        a.ui.unbindKey(toKey(key));
    });
    m.def("_key_bindings", [](nb::object owner) {
        PyApp& a = nb::cast<PyApp&>(owner);
        UiLock l{a};
        std::vector<std::pair<std::string, std::string>> out;
        for (auto const& b : a.ui.keyBindings()) {
            out.emplace_back(sml::keyName(b.key), b.description);
        }
        return out;
    });
}
