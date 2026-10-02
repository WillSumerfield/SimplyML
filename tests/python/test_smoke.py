import numpy as np
import pytest

import simplyml


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
