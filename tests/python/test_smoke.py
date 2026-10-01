import os
import time

import numpy as np
import pytest

import simplyml

needs_display = pytest.mark.skipif(not os.environ.get("DISPLAY"), reason="needs an X display")


def test_store_accepts_scalars_arrays_and_lists(tmp_path):
    s = simplyml.MetricStore()
    s.push("loss", 1.0)
    s.push("loss", 0.5, step=10)
    s.push_many("loss", np.linspace(0, 1, 5, dtype=np.float32))
    s.push_many("acc", [0.1, 0.2], steps=[1, 2])
    with pytest.raises(ValueError):
        s.push_many("acc", [1, 2, 3], steps=[1, 2])
    s.write_csv(str(tmp_path / "m.csv"))  # nothing synced yet: header only
    assert (tmp_path / "m.csv").read_text().splitlines() == ["series,step,value"]


def test_key_names():
    names = simplyml.key_names()
    assert {"a", "0", "space", "escape", "f12"} <= set(names)
    assert len(names) == len(set(names))


def test_custom_events_queue_without_a_window():
    app = simplyml.App()
    app.post_event("hello")
    (e,) = app.poll_events()
    assert (e.type, e.name, e.key, e.button) == ("custom", "hello", None, None)
    assert app.poll_events() == []


def _find_window(display, title, timeout=5.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for w in display.screen().root.query_tree().children:
            if w.get_wm_name() == title:
                return w
        time.sleep(0.05)
    raise TimeoutError(f"window {title!r} never appeared")


@needs_display
def test_window_runs_while_python_pushes_and_sees_keys():
    Xlib = pytest.importorskip("Xlib")
    from Xlib import X, XK
    from Xlib.display import Display
    from Xlib.ext import xtest

    d = Display()
    title = f"simplyml-smoke-{os.getpid()}"
    with simplyml.App(title=title, size=(320, 240)) as app:
        w = _find_window(d, title)
        geo = w.get_geometry()
        pos = w.translate_coords(d.screen().root, 0, 0)
        xtest.fake_input(d, X.MotionNotify, x=-pos.x + geo.width // 2, y=-pos.y + geo.height // 2)
        w.set_input_focus(X.RevertToParent, X.CurrentTime)
        d.sync()

        code = d.keysym_to_keycode(XK.string_to_keysym("space"))
        events = []
        step = 0
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline and not any(e.key == "space" for e in events):
            for _ in range(1000):  # busy Python loop: the UI must not need the GIL
                app.store.push("loss", 1.0 / (step + 1))
                step += 1
            if step == 1000:
                xtest.fake_input(d, X.KeyPress, code)
                xtest.fake_input(d, X.KeyRelease, code)
                d.sync()
            events += app.poll_events()
            time.sleep(0.01)
        assert app.is_running
        assert [e.type for e in events if e.key == "space"][:2] == ["key_pressed", "key_released"]
    assert not app.is_running
    d.close()
