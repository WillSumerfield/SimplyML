# UI

Themed, data-bound widgets and the layout that arranges them into a dashboard.

## Architecture

Sits on `core`. Widgets draw from a Theme and bound data, never from training code directly.

- **theme**: palette, sizes and font, all scaled by one factor.
- **geometry**: batched shape meshes (rounded rects, lines, polylines, arrows, curves).
- **rounded_rect**: a cached card shape with fill, outline and shadow.
- **text**: aligned text drawing.
- **widget**: the widget base and the generic container.
- **layout**: rows, columns and grids.
- **panel**: the dashboard card that most widgets are drawn in.
- **ui**: the widget tree attached to one App.
- **axes**: tick spacing, data-to-pixel mapping and grid/labels shared by charts.
- **value_format**: how numbers are shown.
- **line_chart, bar_chart, phase_plot**: charts of metric series.
- **stats**: value readouts, gauges, status dots and the cards and tiles holding them.
- **controls**: buttons, toggles, sliders, selects and number fields, and the panel stacking them.
- **key_bindings**: hotkeys and the legend panel listing them.
- **ruler, tracer**: scale and trail drawables for world-space scenes.

## Language

**Theme**:
The look shared by every widget: palette, sizes, font and scale.
_Avoid_: Style, skin

**Accent**:
The color identifying one panel or series (its outline, line or bars).

**Widget**:
Anything on screen owning a rectangle of the window; it draws itself there and may react to input.
_Avoid_: Component, element, control (except for interactive widgets)

**Container**:
A widget that places child widgets (Row, Column, Grid) and draws nothing itself.

**Layout**:
Assigning every widget its rectangle from the window size; redone as the window resizes.

**Extent**:
How much room a widget asks for along its row or column: fixed pixels, a share of what is left, or both.
_Avoid_: Weight, flex

**Span**:
How many grid cells a widget covers, across and down.

**Reflow**:
A grid changing its column count to fit the window width.

**Panel**:
The dashboard card: accent outline, dark body, title and an optional readout, with content inside.
_Avoid_: Card, box

**Content Rect**:
The part of a panel inside its outline, padding and title, where its content is drawn.

**Ui**:
The widget tree of one App, laid out to its window and fed its input.
_Avoid_: GUI, screen

**Hover / Press / Focus**:
The pointer being over a widget / held down on it / the last interactive widget pressed, which receives keys.

**Interactive Widget**:
A widget that claims presses (buttons, sliders); others let input pass through.

**Binding**:
A widget naming the Metric Store series it shows; it reads the latest data itself each frame.
_Avoid_: Subscription, feed

**Chart**:
A panel plotting series against their steps (line, bar) or against each other (phase plot).
_Avoid_: Graph, plot (except Phase Plot)

**Window**:
How many of the newest points a chart shows; all of them by default.
_Avoid_: History, buffer

**Readout**:
A panel's latest value, shown at the top-right of its title row.

**Legend**:
Colored series names in a chart's title row, shown when it has several series.

**Value Format**:
How a number is turned into text: decimals, zero padding, percent or duration.

**Gauge**:
A labelled bar showing where a value sits in a fixed range.
_Avoid_: Progress bar, meter

**Status Dot**:
A colored dot and text showing whether something is on or off.

**Stat Card**:
A panel stacking value readouts, gauges and status dots.

**Stat Tile**:
A small untitled panel with one label and one large value.

**Phase Plot**:
A trajectory of one series against another, newest points drawn thickest.

**Tracer**:
A fading trail behind a moving point in a scene.

**Ruler**:
A labelled scale of major and minor ticks, usually under a scene.

**Control**:
An interactive widget editing one named value in the Control Store; its name is also its id.
_Avoid_: Input, parameter widget

**Commit**:
A user edit of a control: the value is published, a change event is queued and callbacks run. Writes from code don't commit.

**Control Panel**:
A panel stacking controls, each at its natural height.

**Select**:
A control choosing one of several options, as a segmented row or a radio list; its value is the option's index.
_Avoid_: Dropdown, combo box

**Number Field**:
A control edited by typing a number, applied on Enter or when it loses focus.

**Key Binding**:
A hotkey registered with the Ui: it runs an action, triggers a control, or only documents a key handled elsewhere.
_Avoid_: Shortcut, hotkey (in code)

**Trigger**:
What a bound key does to a control: press a button, flip a toggle, pick the next option.

**Key Bindings Panel**:
A panel listing every key binding as a key cap and its description.
