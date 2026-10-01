# SimplyML docs

SimplyML gives your training run a live dashboard. Your training code pushes numbers into a store, and a window, running on its own thread, draws them as charts, stats and gauges. You can add sliders, toggles and buttons too, and read them back from your loop to steer training as it runs. It's a C++17 library with Python bindings, and the two share the same API.

## What you can do

- **Get it running**: install a prebuilt Python wheel, build from source, or pull it into a C++ project with CMake. → [install](install.md)
- **Understand the moving parts**: the App, the Metric Store your loop pushes to, the Control Store it reads from, events, and why the window never blocks your loop. → [concepts](concepts.md)
- **Arrange a dashboard**: rows, columns and grids, fixed or shared sizes, and colors. → [layout](layout.md)
- **Show your metrics**: line charts, bar charts, phase plots, stat tiles, stat cards and gauges. → [widgets](widgets.md)
- **Steer training live**: buttons, toggles, sliders, option pickers, number fields and hotkeys, plus callbacks when they change. → [controls](controls.md)
- **Draw ML-specific things**: a network's weights and activations (straight from a torch model if you like), a cart-pendulum scene, and a training-progress card. → [ML widgets](ml.md)
- **Learn from working code**: every example, what it shows and how to run it, including a full NEAT pendulum trainer. → [examples](examples.md)
- **Hack on SimplyML itself**: building, testing, the repo layout and cutting a release. → [development](development.md)

## Doc map

| Doc | What it covers |
|---|---|
| [install.md](install.md) | Wheels, source builds and system packages, supported platforms, C++ via CMake |
| [concepts.md](concepts.md) | App, Metric Store, Control Store, Snapshot, events, threading |
| [layout.md](layout.md) | Rows, columns, grids, sizing, panels, ids, colors and scale |
| [widgets.md](widgets.md) | Charts, stat tiles and cards, gauges, number formats |
| [controls.md](controls.md) | Every control, reading values, keys, events and callbacks |
| [ml.md](ml.md) | Network view, `mlp_graph`, cart-pendulum, training stats |
| [examples.md](examples.md) | What each example shows and how to run it |
| [development.md](development.md) | Build, tests, repo layout, releasing |
