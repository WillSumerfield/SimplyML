"""SimplyML: live ML-training dashboards. The UI runs on its own thread; push data, poll events."""

import time

import numpy as np

from ._core import App as _App
from ._core import Event, MetricStore, key_names

__all__ = ["App", "Event", "MetricStore", "key_names"]


def _push_many(self, name: str, values, steps=None) -> None:
    """Appends many points. `values`/`steps` are any 1-D array-likes; without `steps`, steps
    continue from the series' last step."""
    values = np.ascontiguousarray(values, dtype=np.float64).reshape(-1)
    if steps is not None:
        steps = np.ascontiguousarray(steps, dtype=np.float64).reshape(-1)
    self._push_many(name, values, steps)


MetricStore.push_many = _push_many


class App(_App):
    """Dashboard window. Use as a context manager to start it and close it on exit:

        with simplyml.App(title="run 1") as app:
            for step in range(1000):
                app.store.push("loss", loss)
                for e in app.poll_events(): ...
    """

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
