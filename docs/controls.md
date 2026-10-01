# Controls

Controls let you steer a run while it trains: nudge the learning rate, pause, switch optimizers, save a checkpoint. Every control has a **name**. Its value lives under that name in the [Control Store](concepts.md#control-store), so your loop just reads `app.controls["name"]`. That's one short lock, cheap enough to do every step.

Controls usually go in a `control_panel`, which stacks them neatly, but any container can hold them.

```python
import simplyml

app = simplyml.App()
panel = app.ui.control_panel("Training")
panel.toggle("pause", "Paused", key="space")
panel.slider("lr", "Learning rate", lo=1e-5, hi=1e-1, value=1e-3, log=True)
panel.select("optimizer", ["SGD", "Adam"], label="Optimizer", value="Adam")
panel.number("batch", "Batch size", value=64, lo=1, hi=4096, integer=True)
panel.button("save", "Save checkpoint", key="s")

with app:
    for step in range(200):
        if app.controls["pause"]:
            continue
        lr = app.controls["lr"]
        optimizer = app["optimizer"].value  # "SGD" or "Adam"
```

## Reading and writing values

- `app.controls["lr"]` is always a float. Toggles are `0.0`/`1.0`, selects give the option's index, buttons give their click count.
- `app["lr"].value` gives the natural type: a float, an int, a bool for toggles, or the option string for selects.
- Writing either one (`app.controls["lr"] = 1e-4`) moves the widget to match on the next frame. Handy for schedules.
- `app["lr"].enabled = False` greys a control out and ignores input until you turn it back on.

## The controls

**Button**: `button(name, label)`. Its value counts clicks, so to react to a click, compare it with the last count you saw, or watch for its event (below).

**Toggle**: `toggle(name, label, on=False)`. An on/off switch, for things like pause or data augmentation.

**Slider**: `slider(name, label, lo, hi, value)`. A value in a range.
- `log=True`: spaces values logarithmically. Use it for learning rates.
- `step=1`: snaps to multiples of the step, so `step=1` gives whole numbers.
- `fmt=".2e"`: how the value is shown (see [number formats](widgets.md#number-formats)).

**Select**: `select(name, options, label, value)`. Pick one of a few options. `value` is the starting option, by name or index. It shows as a segmented row, or a vertical list with `radio=True`.

**Number field**: `number(name, label, value, lo, hi)`. Type in an exact number; it applies when you press Enter or click away. Values outside `[lo, hi]` are rejected, and `integer=True` only accepts whole numbers.

## Hotkeys

Pass `key=` to a button, toggle or select, and that key clicks it, flips it, or picks the next option. For anything else, bind a key on the app:

```python
import simplyml

app = simplyml.App()
app.bind_key("escape", "Quit", app.close)  # runs on the window's thread
app.bind_key("h", "Show help")             # no callback: it arrives as an event instead
app.ui.key_bindings()                      # a panel listing every bound key
```

Key names are lowercase strings like `"space"`, `"r"` or `"f1"`; `simplyml.key_names()` lists them all.

## Reacting to changes

There are two ways to hear about a change.

**Events** are the simplest. Each edit queues an event named after the control, which you drain in your loop:

```python
import simplyml

app = simplyml.App()
app.ui.control_panel("Run").button("save", "Save checkpoint")

with app:
    for step in range(100):
        for e in app.poll_events():
            if e.name == "save":
                print("saving checkpoint at step", step)
```

**Callbacks** run right away, on the window's thread. Keep them quick: setting a flag or printing is fine, but training or saving isn't.

```python
import simplyml

app = simplyml.App()
panel = app.ui.control_panel("Run")
panel.slider("lr", "Learning rate", lo=1e-5, hi=1e-1, value=1e-3, log=True,
             on_change=lambda lr: print("lr is now", lr))
panel.button("save", "Save", on_click=lambda clicks: print("clicked", clicks, "times"))
```

Toggles, selects and number fields take `on_change` too.

## C++

```cpp
auto& panel = root.add<sml::ControlPanel>("Training", pal.accent);
auto& pause = panel.addToggle("pause", "Paused");
panel.addSlider("lr", "Learning rate", 1e-5, 1e-1, 1e-3).setLog(true);
panel.addSelect("optimizer", "Optimizer", {"SGD", "Adam"}, 1);
panel.addNumber("batch", "Batch size", 64).setRange(1, 4096).setInteger(true);
panel.addButton("save", "Save checkpoint").onChange([](double clicks) { /* UI thread */ });
root.add<sml::KeyBindings>("Keys");

ui.bindKey(sf::Keyboard::Key::Space, pause);
ui.bindKey(sf::Keyboard::Key::H, "Show help", [] { /* UI thread */ });

double lr = app.controls().get("lr", 1e-3);  // from any thread
```
