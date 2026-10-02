# ML widgets

These widgets show things that aren't plain series: a network's insides, a physics scene, and a summary of training progress. You feed them with setter methods, which are safe to call from your training thread at any rate. The window just shows the newest data each frame.

## Network view

Draws a layered network: one column of nodes per layer, joined by edges. Values drive the drawing. A node's value is usually its activation, and an edge's value is usually its weight. Positive values are drawn green and negative ones red, and bigger magnitudes are drawn bigger.

Options:
- `edge_scale=20`: edge width per unit of edge value. Turn it down if your weights are large.
- `max_zoom=1.5`: caps how much a small network is blown up to fill the panel.
- `footer=False`: hides the layer sizes under the drawing.
- `vertical=True`: lays the layers out as rows from top to bottom, which suits wide panels and tall layers. First-layer labels go above, last-layer labels below.

The easy way to fill it is `simplyml.mlp_graph`. Give it a list of weight matrices shaped `(out, in)`, plus the activations of one sample if you want node values:

```python
import numpy as np

import simplyml

app = simplyml.App()
net = app.ui.network_view("Network", edge_scale=1.5)

rng = np.random.default_rng(0)
weights = [rng.normal(size=(8, 2)), rng.normal(size=(1, 8))]
x = np.array([0.3, -0.5])
hidden = np.tanh(weights[0] @ x)
out = np.tanh(weights[1] @ hidden)
net.set_graph(simplyml.mlp_graph(weights, activations=[x, hidden, out],
                                 input_labels=["x", "y"], output_labels=["score"]))
```

With the torch extra installed, pass a model and one input sample instead. It finds the `nn.Linear` layers and records each layer's output:

<!-- no-test -->
```python
net.set_graph(simplyml.mlp_graph(model, x=batch[0]))
```

For anything that isn't an MLP (NEAT genomes, skip connections...), build a `simplyml.LayeredGraph` yourself. You give it a layer number per node, a list of `(from, to)` edges, and optionally node values, edge values and labels. The view only redoes its layout when the shape of the graph changes, so updating values every step is cheap.

C++: `net.setGraph(sml::LayeredGraph{...})`, with nodes and edges as structs.

## Cart-pendulum

Draws a chain of links hanging from a cart on a rail. It's built for balancing tasks like cart-pole and its double-pendulum cousins. Everything is in your simulation's world units, with y pointing down.

- `rail=(from, to, y)`: where the cart can travel.
- `world=(x, y, w, h)`: the part of the world to show. By default, it's an area around the rail.
- `ruler=False`: hides the distance ruler under the rail.
- `ghost_alpha`: how faded the ghosts are (0–255).

Each step, call `set_state(base, joints, push)`. `base` is the cart's pivot, `joints` is the end of each link from the cart outward, and `push` draws a disturbance arrow at the tip (`0` for none). `set_ghosts(bases, joints)` draws faded extra chains behind it, for example the rest of a population.

```python
import math

import simplyml

app = simplyml.App()
scene = app.ui.cart_pendulum("Pendulum", rail=(-200, 200, 0))

angle = 0.3
base = (0.0, 0.0)
tip = (base[0] + 100 * math.sin(angle), base[1] - 100 * math.cos(angle))  # y down: up is negative
scene.set_state(base, [tip])
scene.set_ghosts([(20.0, 0.0)], [[(40.0, -98.0)]])
```

C++: `scene.setState(sml::LinkChainState{base, joints, push})` and `setGhosts(...)`. You can also give the cart a wheel image with `setWheelTexture`.

## Training stats

A stat card for iterative training: the iteration, best score, simulated time and real time. Push all of them in one call with `store.push_stats`, then add any extra rows with the usual stat card methods:

```python
import time

import simplyml

app = simplyml.App()
card = app.ui.training_stats("Generation", score_label="Best fitness")
card.gauge("Accuracy", "acc", 0, 1, fmt="%")

start = time.monotonic()
for generation in range(10):
    app.store.push_stats(generation, best_score=generation * 1.5, sim_time=generation * 20.0,
                         wall_time=time.monotonic() - start, acc=generation / 10)
```

Extra keywords to `push_stats` (like `acc` here) become series of their own. If you show two training runs side by side, give each one a `prefix` in both places.

C++: `sml::TrainingStatsCard` and `sml::pushStats(app.store(), sml::TrainingStats{...})`.
