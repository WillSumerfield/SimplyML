# Python

The `simplyml` Python package: the native module plus a thin Pythonic layer. Training loops stay in Python; the UI runs on its own thread inside the same process.

## Architecture

Sits on `core` (and later `ui`, `ml`). Exposes the same concepts as C++, renamed to Python conventions.

- **module**: the native module's bindings.
- **python/simplyml**: the importable package wrapping the native module.

## Language

**Native Module**:
The compiled extension (`simplyml._core`) holding the bindings; users import `simplyml`, not it.
_Avoid_: Extension, backend

**Poll**:
Taking all queued App Events from Python in one call, oldest first; the only way input reaches a Python loop.
_Avoid_: Listen, callback

**Key Name**:
The lowercase string identifying a key in events and bindings (e.g. "space", "f1", "7").
