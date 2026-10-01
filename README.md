# SimplyML

A small C++17 library for live ML-training dashboards, built on SFML 3, with Python bindings planned. Training code pushes metrics; SimplyML renders them in dark, rounded, themed widgets.

> Early development. Currently provides the `util` and `core` layers (app shell, events, camera, metric store); widgets come next.

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

Each layer is also its own target, so you can link just what you need: `SimplyML::util` (header-only, no SFML) or `SimplyML::core`.

## Quick look

```cpp
#include <simplyml/core/app.hpp>

sml::App app;                       // windowed 1600x900 by default
app.onDraw([&](sml::Canvas& c) { /* read app.store().series("loss"), draw */ });
app.start();                        // UI on a background thread

for (int step = 0; app.isRunning(); ++step) {
    app.store().push("loss", step, train());   // safe from any thread
}
app.join();
```

`app.run(update)` instead blocks on the calling thread. See `examples/cpp/threaded_training`.

## Credits

SimplyML is extracted from [Pendulum-NEAT](https://github.com/johnBuffer/Pendulum-NEAT) by Jean Tampon (johnBuffer), MIT licensed. Its visual style and much of the original rendering code come from that project.

The default font is [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) by Carrois Type Design, licensed under the SIL Open Font License 1.1 (`res/fonts/OFL.txt`).

## License

MIT; see `LICENSE`.
