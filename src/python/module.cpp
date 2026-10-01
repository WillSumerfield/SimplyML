#include <stdexcept>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "simplyml/core/app.hpp"
#include "simplyml/core/keys.hpp"
#include "simplyml/core/metric_store.hpp"

namespace nb = nanobind;
using namespace nb::literals;

namespace
{

using Array = nb::ndarray<double const, nb::ndim<1>, nb::c_contig, nb::device::cpu>;

char const* typeName(sml::AppEvent::Type t)
{
    using T = sml::AppEvent::Type;
    switch (t) {
        case T::Closed:        return "closed";
        case T::KeyPressed:    return "key_pressed";
        case T::KeyReleased:   return "key_released";
        case T::MousePressed:  return "mouse_pressed";
        case T::MouseReleased: return "mouse_released";
        case T::Custom:        return "custom";
    }
    return "custom";
}

bool isKey(sml::AppEvent const& e)
{
    return e.type == sml::AppEvent::Type::KeyPressed || e.type == sml::AppEvent::Type::KeyReleased;
}

bool isMouse(sml::AppEvent const& e)
{
    return e.type == sml::AppEvent::Type::MousePressed || e.type == sml::AppEvent::Type::MouseReleased;
}

} // namespace

NB_MODULE(_core, m)
{
    m.doc() = "SimplyML native core";

    m.def("key_names", [] {
        std::vector<std::string> out;
        for (unsigned i = 0; i < sf::Keyboard::KeyCount; ++i) {
            out.emplace_back(sml::keyName(static_cast<sf::Keyboard::Key>(i)));
        }
        return out;
    }, "Every valid key name, as used in `Event.key`.");

    nb::class_<sml::AppEvent>(m, "Event", "Input or custom event, drained with `App.poll_events()`.")
        .def_prop_ro("type", [](sml::AppEvent const& e) { return typeName(e.type); },
            "'closed', 'key_pressed', 'key_released', 'mouse_pressed', 'mouse_released' or 'custom'.")
        .def_prop_ro("key", [](sml::AppEvent const& e) -> std::optional<std::string> {
            if (!isKey(e)) {
                return std::nullopt;
            }
            return std::string(sml::keyName(e.key));
        }, "Key name for key events (see `key_names()`), else None.")
        .def_prop_ro("button", [](sml::AppEvent const& e) -> std::optional<std::string> {
            if (!isMouse(e)) {
                return std::nullopt;
            }
            return std::string(sml::buttonName(e.button));
        }, "'left', 'right', 'middle', 'extra1' or 'extra2' for mouse events, else None.")
        .def_prop_ro("position", [](sml::AppEvent const& e) { return std::make_tuple(e.position.x, e.position.y); },
            "Mouse position in window pixels (mouse events).")
        .def_ro("name", &sml::AppEvent::name, "Name of a custom event.")
        .def("__repr__", [](sml::AppEvent const& e) {
            std::string s = std::string("Event(") + typeName(e.type);
            if (isKey(e)) {
                s += ", key='" + std::string(sml::keyName(e.key)) + "'";
            } else if (isMouse(e)) {
                s += ", button='" + std::string(sml::buttonName(e.button)) + "', position=("
                   + std::to_string(e.position.x) + ", " + std::to_string(e.position.y) + ")";
            } else if (!e.name.empty()) {
                s += ", name='" + e.name + "'";
            }
            return s + ")";
        });

    nb::class_<sml::MetricStore>(m, "MetricStore",
        "Named scalar series. Writers are thread-safe and cheap; the UI picks points up each frame.")
        .def(nb::init<>())
        .def("push", [](sml::MetricStore& s, std::string_view name, double value, std::optional<double> step) {
            if (step) {
                s.push(name, *step, value);
            } else {
                s.push(name, value);
            }
        }, "name"_a, "value"_a, "step"_a = nb::none(),
            "Appends one point. Without `step`, uses the previous step of this series + 1 (0 first).")
        .def("_push_many", [](sml::MetricStore& s, std::string_view name, Array values, std::optional<Array> steps) {
            if (steps && steps->shape(0) != values.shape(0)) {
                throw std::invalid_argument("push_many: steps and values differ in length");
            }
            nb::gil_scoped_release release;
            s.pushMany(name, steps ? steps->data() : nullptr, values.data(), values.shape(0));
        }, "name"_a, "values"_a, "steps"_a = nb::none(),
            "float64 C-contiguous arrays only; use `push_many`.")
        .def("set_max_points", &sml::MetricStore::setMaxPoints, "name"_a, "max_points"_a,
            "Keeps only the newest `max_points` of a series (0 = unlimited, the default).")
        .def("clear", &sml::MetricStore::clear, "Drops all series.")
        .def("write_csv", [](sml::MetricStore const& s, std::string const& path) { s.writeCsv(path); }, "path"_a,
            "Writes synced points as CSV (`series,step,value`).")
        .def("write_binary", [](sml::MetricStore const& s, std::string const& path) { s.writeBinary(path); }, "path"_a,
            "Writes synced points in SimplyML's binary format.");

    nb::class_<sml::App>(m, "App",
        "Dashboard window. `start()` runs it on its own thread, so the calling Python loop never blocks it.")
        .def("__init__", [](sml::App* self, std::string title, std::pair<unsigned, unsigned> size, bool fullscreen,
                             unsigned fpsLimit, unsigned antialiasing, bool escToQuit, bool cameraControls,
                             std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> clearColor) {
            sml::AppConfig c;
            c.title          = std::move(title);
            c.size           = {size.first, size.second};
            c.fullscreen     = fullscreen;
            c.fpsLimit       = fpsLimit;
            c.antialiasing   = antialiasing;
            c.escToQuit      = escToQuit;
            c.cameraControls = cameraControls;
            c.clearColor     = {std::get<0>(clearColor), std::get<1>(clearColor), std::get<2>(clearColor)};
            new (self) sml::App(std::move(c));
            self->events().setQueueEnabled(true); // Python reads input through poll_events()
        },
            "title"_a = "SimplyML", "size"_a = std::make_pair(1600u, 900u), "fullscreen"_a = false,
            "fps_limit"_a = 60u, "antialiasing"_a = 4u, "esc_to_quit"_a = false, "camera_controls"_a = true,
            "clear_color"_a = std::make_tuple(std::uint8_t{30}, std::uint8_t{30}, std::uint8_t{30}))
        .def("start", [](sml::App& a) { a.start(); }, "Opens the window and runs the UI on a background thread.")
        .def("close", &sml::App::close, "Asks the UI thread to close the window; returns immediately.")
        .def("join", &sml::App::join, nb::call_guard<nb::gil_scoped_release>(),
            "Blocks until the UI thread ends; re-raises its error, if any.")
        .def_prop_ro("is_running", &sml::App::isRunning)
        .def("poll_events", [](sml::App& a) { return a.events().drain(); },
            "Takes all queued events (unconsumed input, window close, custom), oldest first.")
        .def("post_event", [](sml::App& a, std::string name) {
            sml::AppEvent e;
            e.name = std::move(name);
            a.events().post(std::move(e));
        }, "name"_a, "Queues a custom event, as widgets will.")
        .def("set_fps_limit", &sml::App::setFpsLimit, "fps"_a, "0 = unlimited.")
        .def("set_fullscreen", &sml::App::setFullscreen, "fullscreen"_a)
        .def_prop_ro("store", &sml::App::store, nb::rv_policy::reference_internal);
}
