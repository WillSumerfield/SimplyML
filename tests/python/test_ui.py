import os
import sys
import time

import pytest

import simplyml

needs_display = pytest.mark.skipif(
    sys.platform != "win32" and not os.environ.get("DISPLAY"), reason="needs a display"
)


def build(app):
    root = app.ui.row()
    side = root.column(size=300)
    side.stat_tile("Epoch", "epoch", color="teal", fmt="04d", id="epoch")
    card = side.stat_card("Run", color=(10, 20, 30))
    card.big("Best", "best", fmt=".3")
    card.gauge("LR", "lr", lo=0, hi=1, fmt=".1%")
    card.value("Time", "t", fmt="duration")
    card.status("Aug", "aug", on="On", off="Off")
    grid = root.grid(min_column_width=300, max_columns=3)
    grid.line_chart("Loss", series=["a", "b"], labels=["train", "val"], span=2, id="loss")
    grid.line_chart("Acc", series="acc", y_range=(0, 1), fmt=".1%", window=100)
    grid.bar_chart("Bars", "bars", window=10, id="bars")
    grid.phase_plot("Phase", "x", "y", ranges=(-1, 1, -1, 1))
    grid.gauge("Solo", "lr", weight=2)
    return root, grid


def test_layout_api_without_window():
    app = simplyml.App()
    root, grid = build(app)
    assert len(app.ui) == 1
    assert len(grid) == 5
    assert isinstance(app["loss"], simplyml.Widget)
    assert app["loss"].title == "Loss"
    app["loss"].title = "Loss!"
    assert app["loss"].title == "Loss!"
    assert isinstance(app["epoch"], simplyml.Widget)
    assert isinstance(root, simplyml.Container)
    with pytest.raises(KeyError):
        app["nope"]
    app["bars"].visible = False
    assert not app["bars"].visible
    app["bars"].layout(id="bars2", size=100)
    assert app["bars2"].id == "bars2"


def test_bad_arguments_raise_and_leave_no_widget():
    app = simplyml.App()
    col = app.ui.column()
    with pytest.raises(ValueError):
        col.line_chart("x", color="purple")
    with pytest.raises(ValueError):
        col.line_chart("x", fmt="%%")
    with pytest.raises(TypeError):
        col.line_chart("x", colour="blue")
    with pytest.raises(ValueError):
        col.panel("p", color=(1, 2))
    for bad in (".x", ".2z", "f", ".1.5", "x%", ".123"):
        with pytest.raises(ValueError):
            col.bar_chart("x", fmt=bad)
    for ok in (".3", ".3f", "d", "04d", "%", ".1%", ".2e", "e", ".4g", "g", "duration"):
        col.bar_chart("x", fmt=ok)
    col.clear()
    assert len(col) == 0  # failed calls leave nothing behind
    with pytest.raises(TypeError):
        app.ui.title  # root column is not a panel


def test_value_widget_handles():
    app = simplyml.App()
    card = app.ui.stat_card("c")
    v = card.value("x", "")
    assert isinstance(v, simplyml.ValueWidget)
    v.set_value(3.0)
    v.set_text("hi")
    v.set_series("s")


@needs_display
def test_layout_changes_while_running():
    app = simplyml.App(title=f"ui-{os.getpid()}", size=(640, 480))
    _, grid = build(app)
    with app:
        for i in range(300):
            app.store.push("a", i * 0.1)
            app.store.push("b", -i * 0.1)
            app.store.push("acc", i / 300)
            app.store.push("bars", i % 7)
            app.store.push("x", (i % 20) / 20)
            app.store.push("y", (i % 13) / 13)
            if i == 100:
                grid.line_chart("Late", series="a", id="late")
            if i == 200:
                app["late"].visible = False
            time.sleep(0.002)
        assert app.is_running
        x, y, w, h = app["loss"].bounds
        assert w > 0 and h > 0
    assert not app.is_running
