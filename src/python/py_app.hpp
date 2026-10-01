#pragma once
#include <mutex>

#include <nanobind/nanobind.h>

#include "simplyml/core/app.hpp"
#include "simplyml/ui/ui.hpp"
#include "simplyml/ui/value_format.hpp"

namespace nb = nanobind;

/// What Python's `App` owns: the C++ App plus its widget tree.
struct PyApp
{
    sml::App app;
    sml::Ui  ui;

    PyApp(sml::AppConfig config, sml::Theme theme)
        : app{std::move(config)}
        , ui{app, theme}
    {
        app.events().setQueueEnabled(true); // Python reads input through poll_events()
    }

    ~PyApp()
    {
        // The Ui must not be destroyed under a running loop.
        app.close();
        try {
            nb::gil_scoped_release release;
            app.join();
        } catch (...) {
        }
    }
};

/// Holds the Ui lock; waits for it with the GIL released so the UI thread is never blocked on Python.
struct UiLock
{
    std::unique_lock<std::recursive_mutex> lock;

    explicit UiLock(PyApp& a)
    {
        nb::gil_scoped_release release;
        lock = std::unique_lock{a.ui.mutex()};
    }
};

/// Color from a palette name ("accent", "blue", ...), an (r, g, b[, a]) sequence, or None
/// (transparent = widget default).
sf::Color toColor(nb::handle h, sml::Theme const& theme);

/// Format from a spec: ".3" / ".3f" (decimals), "d" / "04d" (integer, zero-padded), ".1%" (percent),
/// "duration". Anything after the spec in braces-free text is not supported; use prefix/suffix.
sml::ValueFormat toFormat(std::string const& spec);

void bindUi(nb::module_& m);
