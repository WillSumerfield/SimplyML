# Core

The application shell: window, render loop, input, resources and time, plus the thread-safe data path from training code to the UI. Owns everything that was global state in the original engine.

## Architecture

Sits on `util` and SFML; everything above it receives what it needs from an App instead of looking it up globally.

- **app**: the window and render loop, in blocking or background mode.
- **event_bus**: prioritized, consumable input dispatch and the outbound event queue.
- **camera**: world-to-screen mapping with pan and zoom.
- **canvas**: world-space and screen-space drawing for one frame.
- **resources**: named fonts and textures, including the embedded default font.
- **clock**: wall time and sim time.
- **metric_store**: named metric series written from any thread.
- **snapshot**: latest-value state written from any thread.
- **keys**: stable names for keys and mouse buttons.
- **default_font**: the embedded default font, so the library needs no files at runtime.

## Language

**App**:
One dashboard: a window plus everything it renders and reacts to. Several may exist in one process.
_Avoid_: Engine, context, window handler

**UI Thread**:
The thread running an App's loop; the only thread that touches its window, camera, resources and widgets.
_Avoid_: Render thread, main thread

**Frame**:
One pass of the loop: queued tasks, input, sync, update, draw.

**Event Bus**:
Delivers each input event to its subscribers in priority order until one consumes it.
_Avoid_: Event manager, dispatcher

**Consume**:
A subscriber claiming an event, so lower-priority subscribers never see it.

**App Event**:
A plain record of an unconsumed input or a widget action, queued for code outside the UI thread to poll.
_Avoid_: Message, notification

**Key Name**:
The stable lowercase string for a key or mouse button (e.g. "space", "f1", "7", "left"), shared by bindings, legends and Python.
_Avoid_: Key code, scancode

**World Space**:
Coordinates of drawn content (e.g. a simulation), shown through the camera.

**Screen Space**:
Window pixel coordinates, unaffected by the camera; panels and widgets live here.

**Camera**:
The mapping from world space to screen space: a focus point at the view center and a zoom.
_Avoid_: Viewport

**Canvas**:
What draw code receives each frame: somewhere to draw in world space or screen space.
_Avoid_: Render context

**Resources**:
An App's named fonts and textures.

**Wall Time**:
Real elapsed time; drives UI animation, so it never pauses.

**Sim Time**:
Time inside the training or simulation, advanced only by the app; it can pause and be scaled.

**Metric Store**:
The named series that training code writes and the UI reads.
_Avoid_: Logger, history

**Series**:
One named metric as an ordered list of (step, value) points.
_Avoid_: Channel, graph

**Step**:
The x-coordinate of a metric point: iteration, epoch or generation, chosen by the writer.

**Sync**:
The once-per-frame hand-over of newly written points to what the UI reads.

**Snapshot**:
The latest value of some non-series state (e.g. training stats), written from any thread.
_Avoid_: State, shared value
