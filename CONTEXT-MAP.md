# Context Map

## Structure

```
 0 .
 1 ├── build
 1 ├── cmake
 2 │   ├── EmbedFile.cmake
 2 │   └── SimplyMLConfig.cmake.in
 1 ├── CMakeLists.txt
 1 ├── CONTEXT-MAP.md
 1 ├── examples
 2 │   ├── CMakeLists.txt
 2 │   ├── cpp
 3 │   │   ├── controls
 4 │   │   │   └── main.cpp
 3 │   │   ├── dashboard
 4 │   │   │   └── main.cpp
 3 │   │   ├── layout
 4 │   │   │   └── main.cpp
 3 │   │   └── threaded_training
 4 │   │       └── main.cpp
 2 │   └── python
 3 │       ├── controls.py
 3 │       ├── hello_window.py
 3 │       └── train_loop_metrics.py
 1 ├── .gitignore
 1 ├── include
 2 │   └── simplyml
 3 │       ├── core
 4 │       │   ├── app.hpp
 4 │       │   ├── camera.hpp
 4 │       │   ├── canvas.hpp
 4 │       │   ├── clock.hpp
 4 │       │   ├── control_store.hpp
 4 │       │   ├── default_font.hpp
 4 │       │   ├── event_bus.hpp
 4 │       │   ├── keys.hpp
 4 │       │   ├── metric_store.hpp
 4 │       │   ├── resources.hpp
 4 │       │   └── snapshot.hpp
 3 │       ├── ui
 4 │       │   ├── align.hpp
 4 │       │   ├── axes.hpp
 4 │       │   ├── bar_chart.hpp
 4 │       │   ├── controls.hpp
 4 │       │   ├── geometry.hpp
 4 │       │   ├── key_bindings.hpp
 4 │       │   ├── layout.hpp
 4 │       │   ├── line_chart.hpp
 4 │       │   ├── panel.hpp
 4 │       │   ├── phase_plot.hpp
 4 │       │   ├── rounded_rect.hpp
 4 │       │   ├── ruler.hpp
 4 │       │   ├── stats.hpp
 4 │       │   ├── text.hpp
 4 │       │   ├── theme.hpp
 4 │       │   ├── tracer.hpp
 4 │       │   ├── ui.hpp
 4 │       │   ├── value_format.hpp
 4 │       │   └── widget.hpp
 3 │       └── util
 4 │           ├── easing.hpp
 4 │           ├── format.hpp
 4 │           ├── math.hpp
 4 │           ├── ring_buffer.hpp
 4 │           └── smooth_value.hpp
 1 ├── LICENSE
 1 ├── pyproject.toml
 1 ├── .pytest_cache
 2 │   ├── CACHEDIR.TAG
 2 │   ├── README.md
 2 │   └── v
 1 ├── python
 2 │   └── simplyml
 3 │       ├── __init__.py
 3 │       ├── __pycache__
 3 │       └── py.typed
 1 ├── README.md
 1 ├── res
 2 │   └── fonts
 3 │       ├── OFL.txt
 3 │       └── ShareTechMono-Regular.ttf
 1 ├── src
 2 │   ├── core
 3 │   │   ├── app.cpp
 3 │   │   ├── camera.cpp
 3 │   │   ├── CONTEXT.md
 3 │   │   ├── control_store.cpp
 3 │   │   ├── event_bus.cpp
 3 │   │   ├── keys.cpp
 3 │   │   ├── metric_store.cpp
 3 │   │   └── resources.cpp
 2 │   ├── ml
 3 │   │   └── CONTEXT.md
 2 │   ├── python
 3 │   │   ├── CMakeLists.txt
 3 │   │   ├── CONTEXT.md
 3 │   │   ├── control_bindings.cpp
 3 │   │   ├── module.cpp
 3 │   │   ├── py_app.hpp
 3 │   │   ├── ui_bindings.cpp
 3 │   │   └── widget_refs.hpp
 2 │   ├── ui
 3 │   │   ├── axes.cpp
 3 │   │   ├── bar_chart.cpp
 3 │   │   ├── CONTEXT.md
 3 │   │   ├── controls.cpp
 3 │   │   ├── geometry.cpp
 3 │   │   ├── key_bindings.cpp
 3 │   │   ├── layout.cpp
 3 │   │   ├── line_chart.cpp
 3 │   │   ├── panel.cpp
 3 │   │   ├── phase_plot.cpp
 3 │   │   ├── rounded_rect.cpp
 3 │   │   ├── ruler.cpp
 3 │   │   ├── stats.cpp
 3 │   │   ├── text.cpp
 3 │   │   ├── tracer.cpp
 3 │   │   ├── ui.cpp
 3 │   │   ├── value_format.cpp
 3 │   │   └── widget.cpp
 2 │   └── util
 3 │       └── CONTEXT.md
 1 ├── temp
 1 ├── tests
 2 │   ├── CMakeLists.txt
 2 │   ├── core
 3 │   │   ├── test_app.cpp
 3 │   │   ├── test_camera.cpp
 3 │   │   ├── test_clock.cpp
 3 │   │   ├── test_control_store.cpp
 3 │   │   ├── test_default_font.cpp
 3 │   │   ├── test_event_bus.cpp
 3 │   │   ├── test_keys.cpp
 3 │   │   ├── test_metric_store.cpp
 3 │   │   ├── test_resources.cpp
 3 │   │   └── test_snapshot.cpp
 2 │   ├── main.cpp
 2 │   ├── package
 3 │   │   ├── CMakeLists.txt
 3 │   │   └── main.cpp
 2 │   ├── python
 3 │   │   ├── __pycache__
 3 │   │   ├── test_controls.py
 3 │   │   ├── test_smoke.py
 3 │   │   └── test_ui.py
 2 │   ├── ui
 3 │   │   ├── helpers.hpp
 3 │   │   ├── test_controls.cpp
 3 │   │   ├── test_geometry.cpp
 3 │   │   ├── test_layout.cpp
 3 │   │   ├── test_render.cpp
 3 │   │   └── test_values.cpp
 2 │   └── util
 3 │       ├── test_easing.cpp
 3 │       ├── test_format.cpp
 3 │       ├── test_math.cpp
 3 │       ├── test_ring_buffer.cpp
 3 │       └── test_smooth_value.cpp
 1 └── .venv
```

## Contexts

Glossaries live under `src/<layer>/`; public headers for each layer are in `include/simplyml/<layer>/`.

- [Util](./src/util/CONTEXT.md) — render-agnostic math, formatting, history and easing helpers
- [Core](./src/core/CONTEXT.md) — app shell: window, loop, input, resources, time, training→UI data path
- [UI](./src/ui/CONTEXT.md) — themed, data-bound widgets and layout
- [ML](./src/ml/CONTEXT.md) — training-specific widgets and the data formats they consume
- [Python](./src/python/CONTEXT.md) — the `simplyml` package: in-process bindings, UI on its own thread

## Relationships

- **Layering**: Util → Core → UI → ML; each layer depends only on those before it. Util has no SFML dependency.
- **Core → UI**: widgets draw with resources and time handed down from the App, never looked up globally; controls write the App's Control Store, which training code reads.
- **Python → all layers**: binds the C++ API with Python naming; Python reaches the UI only through the Metric Store, Snapshots, the Control Store and polled App Events.
- **ML → UI**: domain widgets are built from UI primitives and consume plain format structs, not training-framework types.
