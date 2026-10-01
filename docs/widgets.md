# Widgets

Display widgets show what's in the [Metric Store](concepts.md#metric-store). Tell a widget which series to show, push to that series, and it updates on its own. Each one is added from a container (see [layout](layout.md)) and takes the usual layout keywords (`size`, `weight`, `id`...) plus a `color`.

This page covers the main options. For the full list, check the docstrings (`help(simplyml.Container.line_chart)`) or the C++ headers in `include/simplyml/ui/`.

## Line chart

Your go-to for anything over time: loss, accuracy, learning rate.

- `series`: one series name, or a list of them to compare (it adds a legend).
- `colors` / `labels`: per-series colors and legend names, when you pass a list.
- `window=500`: only show the newest 500 points. The default, `0`, shows everything.
- `y_range=(0, 1)`: pin the y axis. Otherwise it follows the data. `include_zero=False` lets it leave out zero.
- `area=False`: no fill under the line.

```python
import simplyml

app = simplyml.App()
app.ui.line_chart("Loss", series=["train_loss", "val_loss"], labels=["train", "val"], colors=["accent", "blue"])
app.ui.line_chart("Accuracy", series="acc", y_range=(0, 1), fmt=".1%", window=1000)
```

C++: `LineChart("Loss", "train_loss", pal.accent).addSeries("val_loss", pal.blue, "val")`.

## Bar chart

The newest values of one series as bars. Good for per-epoch or per-episode numbers.

- `window=50`: how many bars to show.
- `fill=True`: stretch the bars across the full width.
- `y_range`: pin the y axis.

```python
import simplyml

app = simplyml.App()
app.ui.bar_chart("Reward per episode", "reward", color="green", window=30)
```

C++: `BarChart("Reward per episode", "reward", pal.green).setWindow(30)`.

## Phase plot

Plots one series against another, pairing their points by index. Use it to see how two quantities move together, like a pendulum's angle against its velocity.

- `x`, `y`: the two series.
- `max_points=500`: how much of the trail to keep.
- `ranges=(xlo, xhi, ylo, yhi)`: pin both axes.

```python
import simplyml

app = simplyml.App()
app.ui.phase_plot("Angle vs velocity", x="angle", y="velocity", max_points=300)
```

C++: `PhasePlot("Angle vs velocity", "angle", "velocity").setMaxPoints(300)`.

## Stat tile

One label and one big number: the latest value of a series. Great for epoch or step counters.

```python
import simplyml

app = simplyml.App()
app.ui.stat_tile("Epoch", "epoch", color="teal", fmt="04d")
```

C++: `StatTile("Epoch", "epoch", ValueFormat::integer(4), pal.teal)`.

## Stat card

A card with several rows. Add rows with its methods:

- `big(label, series)`: a large headline number.
- `value(label, series)`: a smaller labeled number.
- `gauge(label, series, lo, hi)`: a progress bar for a value between `lo` and `hi`.
- `status(label, series, on="Running", off="Idle")`: shows the `on` text while the series' latest value is above 0.5, else `off`.

```python
import simplyml

app = simplyml.App()
card = app.ui.stat_card("Run", color="blue")
card.big("Best val loss", "best_val", fmt=".4")
card.value("Elapsed", "elapsed", fmt="duration")
card.gauge("Progress", "progress", fmt=".0%")
card.status("Validation", "validating", on="Running", off="Idle")
```

C++: `StatCard` with `addBig`, `addValue`, `addGauge` and `addStatus`.

## Gauge

The same progress bar as in a stat card, standing on its own: `container.gauge("Progress", "progress", lo=0, hi=1)`. C++: `Gauge`.

## Showing a value without a series

Value widgets (stat tiles, gauges, stat card rows) can also show something you set directly. `set_value(0.5)` shows a number and `set_text("warming up")` shows text. Either one is used until the series has data.

## Number formats

Anything showing numbers takes `fmt`, a short format string:

| `fmt` | Shows | Example |
|---|---|---|
| `"d"`, `"04d"` | integer, optionally zero-padded | `0042` |
| `".3"` | fixed decimals | `0.123` |
| `".1%"` | percent | `12.3%` |
| `".2e"` | scientific | `1.23e-04` |
| `".4g"` | the shortest of the above | `0.0001234` |
| `"duration"` | seconds as a duration | `2 minutes, 5 seconds` |

Line charts also take `y_fmt` for the axis labels and `x_fmt` for the steps. In C++, use `sml::ValueFormat::integer(4)`, `number(3)`, `percent(1)`, `scientific(2)`, `general(4)` or `duration()`.
