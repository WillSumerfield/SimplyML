# Python

The `simplyml` Python package: the native module plus a thin Pythonic layer. Training loops stay in Python; the UI runs on its own thread inside the same process.

## Architecture

Sits on every C++ layer. Exposes the same concepts as C++, renamed to Python conventions.

- **module**: the native module's bindings: App, events, store.
- **widget_refs**: widget handles and the helpers that create them.
- **ui_bindings**: the layout and display-widget methods.
- **control_bindings**: the control methods, control values and key bindings.
- **ml_bindings**: the ML widget methods and their data setters.
- **python/simplyml**: the importable package wrapping the native module; `ml` holds the Python Layered Graph and the builders of it.

## Language

**Native Module**:
The compiled extension (`simplyml._core`) holding the bindings; users import `simplyml`, not it.
_Avoid_: Extension, backend

**Poll**:
Taking all queued App Events from Python in one call, oldest first; how input and control edits reach a Python loop.
_Avoid_: Listen

**Callback**:
A Python function a control or key binding runs on the UI thread; it must return quickly, as the window waits for it.
_Avoid_: Handler, hook

**Key Name**:
The lowercase string identifying a key in events and bindings (e.g. "space", "f1", "7").

**Handle**:
A Python object pointing at one widget in an App's layout; it keeps the App alive, not the widget.
_Avoid_: Proxy, reference

**Format Spec**:
The short string choosing a Value Format from Python (".3", "04d", ".1%", ".2e", ".4g", "duration").

**Weights Graph**:
A Layered Graph built from an MLP's weight matrices (or a torch model), with edges valued by weight and nodes by one sample's activations.
