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
