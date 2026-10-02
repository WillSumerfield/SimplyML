# ML

Domain widgets for machine-learning training, and the plain data formats they consume.

## Architecture

Sits on `ui`. Formats are plain structs so any training framework can produce them without depending on widgets. Widgets take formats through thread-safe setters, so a training thread hands them over directly.

- **formats**: the data formats and the helper pushing training stats into the Metric Store.
- **network_view**: the network drawing, for Layered and Placed Graphs.
- **cart_pendulum_view**: the cart-pendulum scene.
- **training_stats_card**: the stat card preset for training progress.

## Language

**Layered Graph**:
A network given as nodes, each in a layer, joined by edges; every node and edge carries one signed value.
_Avoid_: Topology, net

**Placed Graph**:
A network given as nodes, each at a position the user chooses, joined by edges; every node and edge carries one signed value. For networks that aren't layered.
_Avoid_: Free graph, positioned graph

**Topology**:
The parts of a graph that place it on screen: its nodes' layers (Layered Graph) or positions (Placed Graph), their labels, and which nodes the edges join. Values are not part of it.

**Link Chain State**:
A snapshot of a chain of rigid links hanging from a base on a rail: the base, the end of each link, and any push on the tip.
_Avoid_: Agent, pendulum state

**Ghost**:
A faded extra chain drawn behind the main one, e.g. another member of a population.

**Push**:
A short horizontal disturbance applied to the chain's tip, shown as an arrow.
_Avoid_: Force, perturbation

**Training Stats**:
Progress of an iterative trainer: iteration, best score, simulated and real training time, plus any named extras.

**Iteration**:
One round of training (a generation, an epoch); Training Stats count them.
_Avoid_: Step (that is one Metric Store point)
