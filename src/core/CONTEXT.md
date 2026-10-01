# Core

The application shell: window, render loop, input, resources and time, plus the thread-safe data path from training code to the UI. Owns everything that was global state in the original engine.

## Architecture

Sits on `util` and SFML; everything above it receives what it needs from an App instead of looking it up globally.

- **default_font**: the embedded default font, so the library needs no files at runtime.

## Language
