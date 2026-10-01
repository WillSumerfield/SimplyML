# Util

Generic, render-agnostic helpers shared by every other layer. Has no SFML dependency and holds no global state.

## Architecture

The bottom layer: header-only math and time-shaping primitives that `core`, `ui` and `ml` build on.

- **math**: scalar and 2D-vector helpers.
- **format**: number and duration to text.
- **ring_buffer**: fixed-capacity history with running aggregates.
- **easing**: curves that map normalized progress to an eased ratio.
- **smooth_value**: values that ease toward a target over time.

## Language

**Easing**:
A curve mapping normalized progress in [0,1] to an eased ratio that is exactly 0 at the start and 1 at the end; some curves overshoot in between.
_Avoid_: Interpolation, tween

**Smooth Value**:
A value that eases from its current state toward a target over a fixed duration, sampled at a caller-supplied time.
_Avoid_: Transition, animated value
