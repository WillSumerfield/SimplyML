import os
import time

import pytest

import simplyml

needs_display = pytest.mark.skipif(not os.environ.get("DISPLAY"), reason="needs an X display")


def build(app, log=None):
    panel = app.ui.control_panel("Training", color="accent", id="panel")
    panel.toggle("pause", "Paused", key="space", on_change=None if log is None else log.append)
    panel.slider("lr", "Learning rate", lo=1e-5, hi=1e-1, value=1e-3, log=True)
    panel.slider("layers", lo=1, hi=8, value=3, step=1)
    panel.select("opt", ["sgd", "adam"], value="adam", key="o")
    panel.number("batch", "Batch", value=64, lo=1, hi=4096, integer=True)
    panel.button("reset", "Reset", color="accent", on_click=None if log is None else (lambda n: log.append(("reset", n))))
    app.ui.key_bindings()
    return panel


def test_controls_without_window():
    app = simplyml.App()
    panel = build(app)
    assert isinstance(panel, simplyml.Container)
    assert len(panel) == 6
    c = app.controls
    assert c.to_dict() == {"pause": 0.0, "lr": pytest.approx(1e-3), "layers": 3.0, "opt": 1.0, "batch": 64.0, "reset": 0.0}

    assert app["pause"].value is False
    assert app["layers"].value == 3 and isinstance(app["layers"].value, int)
    assert app["lr"].value == pytest.approx(1e-3)
    assert app["opt"].value == "adam" and app["opt"].index == 1 and app["opt"].options == ["sgd", "adam"]
    assert app["batch"].value == 64 and isinstance(app["batch"].value, int)
    assert app["reset"].value == 0

    app["opt"].value = "sgd"
    assert c["opt"] == 0.0
    app["pause"].value = True
    assert c["pause"] == 1.0
    c["lr"] = 0.01
    assert app["lr"].value == 0.01
    assert c.get("nope") is None and c.get("nope", 2.0) == 2.0 and "lr" in c
    with pytest.raises(KeyError):
        c["nope"]

    assert app.key_bindings == [("space", "Paused"), ("o", "opt")]
    app.bind_key("s", "Save")
    app.bind_key("o", "Optimizer")  # rebinding replaces
    assert app.key_bindings == [("space", "Paused"), ("o", "Optimizer"), ("s", "Save")]
    app.unbind_key("s")
    assert len(app.key_bindings) == 2

    app["pause"].enabled = False
    assert not app["pause"].enabled
    app["pause"].label = "Hold"
    assert app["pause"].label == "Hold"


def test_existing_value_wins_and_bad_arguments_add_nothing():
    app = simplyml.App()
    app.controls["lr"] = 0.05
    col = app.ui.column()
    col.slider("lr", lo=0, hi=1, value=0.5)
    assert app.controls["lr"] == 0.05
    n = len(col)
    with pytest.raises(ValueError):
        col.toggle("t", key="notakey")
    with pytest.raises(ValueError):
        col.select("s", ["a"], value="b")
    with pytest.raises(ValueError):
        col.select("s", [])
    with pytest.raises(ValueError):
        col.slider("s", lo=0, hi=1, log=True)
    with pytest.raises(TypeError):
        col.toggle("t", on_change=3)
    with pytest.raises(ValueError):
        col.number("n", fmt="bad")
    assert len(col) == n
    with pytest.raises(ValueError):
        app.bind_key("notakey", "x")


def _find_window(display, title, timeout=5.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for w in display.screen().root.query_tree().children:
            if w.get_wm_name() == title:
                return w
        time.sleep(0.05)
    raise TimeoutError(f"window {title!r} never appeared")


def _wait(cond, timeout=5.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if cond():
            return True
        time.sleep(0.02)
    return False


@needs_display
def test_live_controls_keys_clicks_callbacks():
    pytest.importorskip("Xlib")
    from Xlib import X, XK
    from Xlib.display import Display
    from Xlib.ext import xtest

    d = Display()
    title = f"simplyml-controls-{os.getpid()}"
    app = simplyml.App(title=title, size=(640, 800))
    log = []
    build(app, log)
    saves = []
    app.bind_key("s", "Save", lambda: saves.append(1))
    with app:
        w = _find_window(d, title)
        pos = w.translate_coords(d.screen().root, 0, 0)
        w.set_input_focus(X.RevertToParent, X.CurrentTime)
        d.sync()

        def key(name):
            code = d.keysym_to_keycode(XK.string_to_keysym(name))
            xtest.fake_input(d, X.KeyPress, code)
            xtest.fake_input(d, X.KeyRelease, code)
            d.sync()

        key("space")
        assert _wait(lambda: app.controls["pause"] == 1.0)
        assert _wait(lambda: log == [True])
        key("s")
        assert _wait(lambda: saves == [1])

        assert _wait(lambda: app["reset"].bounds[2] > 0)
        x, y, bw, bh = app["reset"].bounds
        xtest.fake_input(d, X.MotionNotify, x=-pos.x + int(x + bw / 2), y=-pos.y + int(y + bh / 2))
        d.sync()
        time.sleep(0.1)
        xtest.fake_input(d, X.ButtonPress, 1)
        xtest.fake_input(d, X.ButtonRelease, 1)
        d.sync()
        assert _wait(lambda: app["reset"].value == 1)
        assert _wait(lambda: ("reset", 1) in log)

        app["layers"].value = 99  # sanitized by the widget on the next frame
        assert _wait(lambda: app.controls["layers"] == 8.0)

        events = [e for e in app.poll_events() if e.type == "custom"]
        assert [(e.name, e.value) for e in events] == [("pause", 1.0), ("reset", 1.0)]
    d.close()
