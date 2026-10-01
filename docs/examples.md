# Examples

The examples live in `examples/`. Each one is short and runnable, so they're a good place to copy from.

## Python

Run them with `python examples/python/<name>.py`, after [installing](install.md). They need numpy, which SimplyML installs for you.

| Example | What it shows |
|---|---|
| `hello_window.py` | The smallest thing that works: push a fake loss, print key presses from `poll_events()`, and save the metrics to CSV when you close the window. Q quits. |
| `train_loop_metrics.py` | A fake training loop feeding a full dashboard: a stat tile, a stat card with gauges, durations and a status row, multi-series line charts and a bar chart. It retitles a chart when training finishes, then waits for you to close the window. |
| `controls.py` | A small numpy network fitting a sine wave that you steer live. Drag the learning rate, switch optimizers, change the batch size, pause with Space or reset with R. Every control is in there. |
| `network_view.py` | A numpy network learning to tell points inside a ring from points outside. The network view shows its weights and its activations for a probe point circling the ring, next to a training stats card. |

## C++

The C++ examples are built along with SimplyML when it's the top-level project:

```sh
cmake -S . -B build && cmake --build build -j
build/examples/dashboard
```

| Example | What it shows |
|---|---|
| `threaded_training` | The core idea without any widgets: the window runs on a background thread while the main thread "trains" and pushes metrics. |
| `layout` | A fixed-height header row over a grid whose column count follows the window width. Resize the window to watch it reflow. |
| `dashboard` | Every display widget, fed by a fake training loop. |
| `controls` | Every control steering a fake training loop, with hotkeys (Space pauses, R resets, O cycles the optimizer) and a key bindings panel. |
| `network_view` | A small network learning the inside-the-ring task, drawn live, with a training stats card. |
| `cart_pendulum` | A double pendulum on a cart, simulated in the App's per-frame update. Drive the cart with a slider, kick the tip with P, and turn on ghosts with G to watch nearly identical starts drift apart. |

Esc quits any of them.

## Pendulum-NEAT

`examples/pendulum_neat` is the original [Pendulum-NEAT](https://github.com/johnBuffer/Pendulum-NEAT) app, rebuilt on SimplyML. NEAT evolves neural networks that balance a double pendulum on a cart. Each time the population masters the task, gravity goes up and friction goes down. Training runs on its own threads, and the dashboard follows along with live charts, the best network, and a demo that replays the best agent against a fixed set of disturbances.

```sh
build/examples/pendulum_neat                    # windowed, 1600x900
build/examples/pendulum_neat --size 2560x1440 --fullscreen
build/examples/pendulum_neat --load pendulum_neat_out/<genome> --demo
```

Other options: `--threads N` sets the training threads, `--out DIR` sets where genomes are saved (default `./pendulum_neat_out`), and `--conf CONF` goes with `--load`.

| Key | Does |
|---|---|
| D | Switch between training and the demo |
| A | AI on/off (demo) |
| P | Disturbances on/off (demo) |
| B | Show only the best agent (demo) |
| S | Fast demo |
| Space | Next difficulty |
| Backspace | Lower friction |
| W | Save all genomes |
| H | Hide/show the controls |
| Esc | Quit |

It's also the biggest example of using SimplyML from C++: `dashboard.cpp` builds both screens and shows how controls, hotkeys and widgets fit together in a real app.
