# SimplyML

A small C++17 library for live ML-training dashboards, built on SFML 3, with Python bindings. Training code pushes metrics; SimplyML renders them in dark, rounded, themed widgets.

> Early development. Provides the app shell, metric store, layout and display widgets (charts, stat cards, gauges); interactive controls and ML widgets come next.

## Build

```sh
cmake -S . -B build          # Release by default; fetches SFML 3 (static) and doctest
cmake --build build -j
ctest --test-dir build       # unit tests + install/find_package round-trip
```

Linux needs the SFML system dependencies: X11 (`libx11-dev libxrandr-dev libxcursor-dev libxi-dev`), OpenGL and `libudev-dev`. Freetype and HarfBuzz are bundled.

## Use from CMake

```cmake
find_package(SimplyML REQUIRED)            # after `cmake --install build --prefix <prefix>`
target_link_libraries(app PRIVATE SimplyML::SimplyML)
```

Each layer is also its own target, so you can link just what you need: `SimplyML::util` (header-only, no SFML), `SimplyML::core` or `SimplyML::ui`.

## Quick look

```cpp
#include <simplyml/ui/line_chart.hpp>
#include <simplyml/ui/stats.hpp>
#include <simplyml/ui/ui.hpp>

sml::App app;                                   // windowed 1600x900 by default
sml::Ui  ui{app};
auto& grid = ui.setRoot<sml::Grid>(2);
grid.add<sml::LineChart>("Loss", "loss");       // widgets bind to store series by name
grid.add<sml::StatTile>("Epoch", "epoch", sml::ValueFormat::integer(4));
app.start();                                    // UI on a background thread

for (int step = 0; app.isRunning(); ++step) {
    app.store().push("loss", step, train());    // safe from any thread
}
app.join();
```

`app.run(update)` instead blocks on the calling thread. Controls (`Button`, `Toggle`, `Slider`, `Select`, `NumberField`, in a `ControlPanel`) publish to `app.controls()`, which training code reads from any thread; `ui.bindKey` adds hotkeys, listed by a `KeyBindings` panel. See `examples/cpp/dashboard`, `controls`, `layout` and `threaded_training`.

## Python

```sh
uv venv --python 3.12 && uv pip install -e ".[test]"   # builds the C++ core (first build takes ~1 min)
uv run pytest                                          # X display needed for the window test
```

```python
import simplyml

app = simplyml.App(title="run 1")
grid = app.ui.grid(columns=2)
grid.line_chart("Loss", series=["train_loss", "val_loss"], span=2)
grid.stat_tile("Epoch", "epoch", fmt="04d")
panel = grid.control_panel("Controls")
panel.slider("lr", "Learning rate", lo=1e-5, hi=1e-1, value=1e-3, log=True)
panel.toggle("pause", "Paused", key="space")
panel.button("save", "Save checkpoint")

with app:                                        # UI runs on its own thread
    for step in range(10_000):
        if app.controls["pause"]: continue       # control values, readable every step
        app.store.push("train_loss", train_step(lr=app.controls["lr"]))  # never blocks on rendering
        for e in app.poll_events():              # control edits, unconsumed keys/mouse, window close
            if e.name == "save": save()
```

Python 3.10–3.12 on Linux or Windows. Re-run `uv pip install -e .` after changing C++ code. See `examples/python/train_loop_metrics.py` and `controls.py`.

## Credits

SimplyML is extracted from [Pendulum-NEAT](https://github.com/johnBuffer/Pendulum-NEAT) by Jean Tampon (johnBuffer), MIT licensed. Its visual style and much of the original rendering code come from that project.

The default font is [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) by Carrois Type Design, licensed under the SIL Open Font License 1.1 (`res/fonts/OFL.txt`).

## License

MIT; see `LICENSE`.
