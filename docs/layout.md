# Layout

A dashboard is a tree of containers with widgets inside. `app.ui` is the root, a column that fills the window. Every container method adds a child and hands it back, so you build the tree by nesting calls.

## Rows, columns and grids

- `row()` puts children side by side.
- `column()` stacks them top to bottom.
- `grid(columns=2)` fills cells left to right, then wraps to the next row.

```python
import simplyml

app = simplyml.App()
root = app.ui.row()
side = root.column(size=320)  # a fixed-width sidebar
side.stat_tile("Epoch", "epoch")
side.stat_tile("Best", "best", fmt=".4")

grid = root.grid(columns=2)  # takes the rest of the width
grid.line_chart("Loss", series="loss", span=2)  # spans both columns
grid.line_chart("Accuracy", series="acc")
grid.bar_chart("Reward", "reward")
```

Want the grid to reflow when the window resizes? Give it `min_column_width=300` instead of a column count, and it fits as many columns as it can (cap it with `max_columns`). `row_height` fixes the height of every grid row. Rows and columns take `gap` and `padding` too.

## Sizing

By default, children of a row or column split the space evenly. Every widget-adding method takes these keywords to change that:

- `size=200`: a fixed size in pixels, along the container's direction (width in a row, height in a column).
- `weight=2`: a share of the leftover space. A weight-2 widget gets twice what a weight-1 one does. Combine it with `size` to get a minimum plus a share.
- `fit=True`: just as big as its content needs. Control panels, stat tiles and stat cards do this by default.
- `span=(2, 1)`: in a grid, how many columns and rows a widget covers.
- `visible=False`: hidden, and it takes up no space.

## Finding and changing widgets later

Give a widget an `id` and grab it again with `app["id"]`. You can change the layout while the app is running:

```python
import simplyml

app = simplyml.App()
app.ui.line_chart("Loss", series="loss", id="loss")
app.ui.panel("Notes", id="notes")

app["loss"].title = "Loss (epoch 3)"
app["notes"].visible = False
app["loss"].layout(weight=3)  # any of the sizing keywords
app.ui.clear()                # remove everything and start over
```

`panel(title)` gives you an empty card, which is handy as a placeholder or spacer.

## Colors and scale

Most widgets take a `color`. Use a palette name (`"accent"`, `"orange"`, `"yellow"`, `"green"`, `"teal"`, `"blue"`, `"grey"`, `"white"`) or an RGB(A) tuple like `(255, 120, 0)`. To make everything bigger or smaller (text, padding, outlines), pass `scale` to the App: `simplyml.App(scale=1.5)`.

## C++

The same tree in C++. Sizes go through `setExtent` with `sml::px(...)`, `sml::fr(...)` (a share) or `sml::fit()`, and grid spans through `setSpan`. Colors come from the theme's palette.

```cpp
sml::Ui ui{app};
auto const& pal = ui.theme().palette;
auto& root = ui.setRoot<sml::Row>();
auto& side = root.add<sml::Column>();
side.setExtent(sml::px(320));
side.add<sml::StatTile>("Epoch", "epoch", sml::ValueFormat::integer(), pal.teal);

auto& grid = root.add<sml::Grid>(2);
grid.add<sml::LineChart>("Loss", "loss").setSpan(2).setId("loss");
grid.add<sml::LineChart>("Accuracy", "acc", pal.green);
```

Change the theme (`ui.theme().scale`, `ui.theme().palette`) before you start the app. Once it's running, hold `ui.mutex()` while you change the layout.
