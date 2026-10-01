# Concepts

There are only a handful of pieces to know. Your training loop talks to two stores: it **pushes** metrics into one and **reads** control values from the other. The window takes care of everything else on its own thread.

```
your training loop ──push──▶  Metric Store  ──▶ charts, stats, gauges
                   ◀──read──  Control Store ◀── sliders, toggles, buttons
                   ◀──poll──  events        ◀── key presses, clicks, window close
```

## App

The App owns the window. Build your layout on `app.ui`, then start it. Using the App as a context manager (`with app:`) starts the window and closes it when the block ends.

```python
import simplyml

app = simplyml.App(title="run 1", size=(1280, 720))
app.ui.line_chart("Loss", series="loss")

with app:
    for step in range(500):
        app.store.push("loss", 1 / (step + 1))
    # app.wait() here would keep the window open until you close it
```

A few constructor options worth knowing:

- `size=(w, h)` and `fullscreen=True`: window size, or the whole screen.
- `scale=1.5`: scales text, padding and outlines. Handy on high-DPI screens.
- `esc_to_quit=True`: Esc closes the window.
- `fps_limit=60`: how often it redraws. `0` means as fast as possible.

If you don't use `with`, call `app.start()` to open the window, `app.close()` to ask it to close, and `app.join()` to wait for it. `app.is_running` tells you whether it's still open, which makes a nice loop condition: `while app.is_running:`.

In C++, `app.start()` works the same way. `app.run()` runs the window on the calling thread instead, and returns when it closes. Both take an optional `update(app, dt)` callback that runs once per frame on the window's thread.

```cpp
sml::AppConfig config;
config.title = "run 1";
sml::App app{config};
sml::Ui  ui{app};
ui.setRoot<sml::LineChart>("Loss", "loss");
app.start();
for (int step = 0; step < 500; ++step) {
    app.store().push("loss", step, 1.0 / (step + 1));
}
app.join();
```

## Metric Store

The Metric Store holds named series of `(step, value)` points. Widgets show a series by its name, so a chart showing `"loss"` picks up whatever you push to `"loss"`. Pushing is cheap and safe from any thread, and it never waits on drawing.

```python
import simplyml

store = simplyml.MetricStore()  # every App has one already: app.store
store.push("loss", 0.9)                # step 0
store.push("loss", 0.7)                # step 1: steps count up on their own
store.push("val_loss", 0.8, step=100)  # or pass your own
store.push_many("acc", [0.1, 0.2, 0.3])  # lists or numpy arrays
store.set_max_points("loss", 10_000)   # keep only the newest points
store.write_csv("metrics.csv")         # series,step,value
store.clear()                          # drop everything (or store.clear("loss"))
```

In C++ the step comes before the value: `app.store().push("loss", step, value)`.

## Control Store

Every control (slider, toggle, button...) has a name, and its current value lives under that name in the Control Store. Read it from your loop as often as you like:

```python
import simplyml

app = simplyml.App()
panel = app.ui.control_panel("Training")
panel.slider("lr", "Learning rate", lo=1e-4, hi=1e-1, value=1e-3, log=True)
panel.toggle("pause", "Paused")

lr = app.controls["lr"]         # always a float
paused = app.controls["pause"]  # toggles are 0.0 or 1.0
app.controls["lr"] = 3e-3       # writing works too; the slider moves to match
```

`app["lr"].value` gives the same value with its natural type: a bool for toggles, an option string for selects. See [controls](controls.md) for each control.

C++: `app.controls().get("lr", 1e-3)` and `app.controls().set("lr", 3e-3)`.

## Snapshot (C++)

For state that isn't a series, like a whole network graph or a physics state, C++ has `sml::Snapshot<T>`. You `set()` it from any thread, and the UI reads the latest one each frame. The ML widgets use it behind their setters, which is why `net.set_graph(...)` is safe to call from your training thread.

## Events

Key presses, mouse clicks, control edits and the window closing all end up in a queue. Drain it with `app.poll_events()`:

```python
import simplyml

app = simplyml.App()
panel = app.ui.control_panel("Run")
panel.button("save", "Save checkpoint")

with app:
    for step in range(100):
        for e in app.poll_events():
            if e.type == "closed" or e.key == "q":
                app.close()
            elif e.name == "save":  # control edits arrive as type "custom", named after the control
                print("saving at step", step)
```

Each event has `type` (`"key_pressed"`, `"mouse_pressed"`, `"custom"`, `"closed"`...), plus `key`, `button`, `position`, `name` and `value` where they apply. Key names are lowercase strings like `"space"` or `"q"`; `simplyml.key_names()` lists them all. Keys and clicks that a widget already handled don't show up here.

In C++, subscribe to window events directly: `app.events().on<sf::Event::KeyPressed>([](auto const& k) { ... })`.

## Threading

The window always runs on its own thread, so a slow training step never freezes it, and resizing the window never slows training down. A few things to keep in mind:

- Pushing metrics, reading controls and polling events are safe from any thread.
- You can change the layout while it's running (add widgets, hide them, retitle them); SimplyML locks around it for you in Python. In C++, hold `ui.mutex()` while you do that.
- Callbacks like `on_change`, `on_click` and key callbacks run on the window's thread. Keep them quick, or the window stutters. For anything slow, set a flag and do the work in your loop.
