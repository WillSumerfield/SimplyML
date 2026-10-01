"""SimplyML: live ML-training dashboards. The UI runs on its own thread; push data, poll events."""

import time

import numpy as np

from . import _core
from ._core import App as _App
from ._core import (CartPendulumView, Container, Control, Controls, Event, MetricStore, NetworkView, StatCard,
                    ValueWidget, Widget, key_names)
from .ml import LayeredGraph, mlp_graph

__all__ = [
    "App", "CartPendulumView", "Container", "Control", "Controls", "Event", "LayeredGraph", "MetricStore",
    "NetworkView", "StatCard", "ValueWidget", "Widget", "key_names", "mlp_graph",
]


def _push_many(self, name: str, values, steps=None) -> None:
    """Appends many points. `values`/`steps` are any 1-D array-likes; without `steps`, steps
    continue from the series' last step."""
    values = np.ascontiguousarray(values, dtype=np.float64).reshape(-1)
    if steps is not None:
        steps = np.ascontiguousarray(steps, dtype=np.float64).reshape(-1)
    self._push_many(name, values, steps)


MetricStore.push_many = _push_many


class App(_App):
    """Dashboard window. Lay it out through `app.ui`, then use it as a context manager to start it
    and close it on exit:

        app = simplyml.App(title="run 1")
        grid = app.ui.grid(columns=2)
        grid.line_chart("Loss", series=["train_loss", "val_loss"], span=2)
        grid.stat_tile("Epoch", "epoch", fmt="04d")
        with app:
            for step in range(1000):
                app.store.push("train_loss", loss)
                for e in app.poll_events(): ...

    Controls (sliders, toggles, buttons, ...) are read back by name, from any thread:

        panel = app.ui.control_panel("Training")
        panel.slider("lr", "Learning rate", lo=1e-5, hi=1e-1, value=1e-3, log=True)
        panel.toggle("pause", "Paused", key="space")
        ...
        lr = app.controls["lr"]          # or app["lr"].value
        for e in app.poll_events():      # edits also arrive as Event(custom, name=..., value=...)
            ...

    The layout may also change while running; widgets are found again with `app["id"]`.
    """

    @property
    def ui(self) -> Container:
        """Root of the layout: a column filling the window."""
        return _core._ui_root(self)

    def __getitem__(self, id: str) -> Widget:
        """Widget with this `id`."""
        return _core._ui_find(self, id)

    def bind_key(self, key: str, description: str, callback=None) -> None:
        """Lists `key` in key_bindings panels. With `callback()` (run on the UI thread; keep it
        quick) the key is handled there; without, it still arrives through `poll_events()`."""
        _core._bind_key(self, key, description, callback)

    def unbind_key(self, key: str) -> None:
        _core._unbind_key(self, key)

    @property
    def key_bindings(self) -> list[tuple[str, str]]:
        """(key, description) of every binding, in order."""
        return _core._key_bindings(self)

    def wait(self, poll: float = 0.05) -> None:
        """Blocks until the window closes. Unlike `join()`, Ctrl-C still works."""
        while self.is_running:
            time.sleep(poll)
        self.join()

    def __enter__(self) -> "App":
        self.start()
        return self

    def __exit__(self, *exc) -> None:
        self.close()
        self.join()
