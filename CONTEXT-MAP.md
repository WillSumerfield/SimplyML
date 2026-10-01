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
 2 │   └── cpp
 3 │       └── threaded_training
 4 │           └── main.cpp
 1 ├── .gitignore
 1 ├── include
 2 │   └── simplyml
 3 │       ├── core
 4 │       │   ├── app.hpp
 4 │       │   ├── camera.hpp
 4 │       │   ├── canvas.hpp
 4 │       │   ├── clock.hpp
 4 │       │   ├── default_font.hpp
 4 │       │   ├── event_bus.hpp
 4 │       │   ├── metric_store.hpp
 4 │       │   ├── resources.hpp
 4 │       │   └── snapshot.hpp
 3 │       └── util
 4 │           ├── easing.hpp
 4 │           ├── format.hpp
 4 │           ├── math.hpp
 4 │           ├── ring_buffer.hpp
 4 │           └── smooth_value.hpp
 1 ├── LICENSE
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
 3 │   │   ├── event_bus.cpp
 3 │   │   ├── metric_store.cpp
 3 │   │   └── resources.cpp
 2 │   ├── ml
 3 │   │   └── CONTEXT.md
 2 │   ├── ui
 3 │   │   └── CONTEXT.md
 2 │   └── util
 3 │       └── CONTEXT.md
 1 ├── temp
 1 └── tests
 2     ├── CMakeLists.txt
 2     ├── core
 3     │   ├── test_app.cpp
 3     │   ├── test_camera.cpp
 3     │   ├── test_clock.cpp
 3     │   ├── test_default_font.cpp
 3     │   ├── test_event_bus.cpp
 3     │   ├── test_metric_store.cpp
 3     │   ├── test_resources.cpp
 3     │   └── test_snapshot.cpp
 2     ├── main.cpp
 2     ├── package
 3     │   ├── CMakeLists.txt
 3     │   └── main.cpp
 2     └── util
 3         ├── test_easing.cpp
 3         ├── test_format.cpp
 3         ├── test_math.cpp
 3         ├── test_ring_buffer.cpp
 3         └── test_smooth_value.cpp
```

## Contexts

Glossaries live under `src/<layer>/`; public headers for each layer are in `include/simplyml/<layer>/`.

- [Util](./src/util/CONTEXT.md) — render-agnostic math, formatting, history and easing helpers
- [Core](./src/core/CONTEXT.md) — app shell: window, loop, input, resources, time, training→UI data path
- [UI](./src/ui/CONTEXT.md) — themed, data-bound widgets and layout
- [ML](./src/ml/CONTEXT.md) — training-specific widgets and the data formats they consume

## Relationships

- **Layering**: Util → Core → UI → ML; each layer depends only on those before it. Util has no SFML dependency.
- **Core → UI**: widgets draw with resources and time handed down from the App, never looked up globally.
- **ML → UI**: domain widgets are built from UI primitives and consume plain format structs, not training-framework types.
