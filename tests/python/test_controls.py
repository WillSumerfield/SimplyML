import pytest

import simplyml


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
