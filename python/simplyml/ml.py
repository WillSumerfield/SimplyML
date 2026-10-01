"""Data formats for the ML widgets, and helpers that build them."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence

import numpy as np

from ._core import CartPendulumView, MetricStore, NetworkView

__all__ = ["LayeredGraph", "mlp_graph"]


@dataclass
class LayeredGraph:
    """A network as layers of nodes joined by edges. Node i is in `layers[i]` (any ints; only their
    order matters); edge k goes from node `edges[k][0]` to `edges[k][1]`. Values set color (sign)
    and size (magnitude): node values are activations, edge values weights or signals."""

    layers: Sequence[int]
    edges: Sequence[tuple[int, int]] | np.ndarray = ()
    values: Sequence[float] | None = None
    edge_values: Sequence[float] | None = None
    labels: Sequence[str] | None = None


def _set_graph(self: NetworkView, graph: LayeredGraph | None = None, **fields) -> None:
    """Shows `graph` (or a LayeredGraph built from the keyword fields). Thread-safe and cheap
    when only values change: positions are recomputed only when the topology does."""
    g = graph if graph is not None else LayeredGraph(**fields)
    edges = np.asarray(g.edges, dtype=np.int32).reshape(-1, 2)
    as_f32 = lambda v: None if v is None else np.ascontiguousarray(v, dtype=np.float32).reshape(-1)
    self._set_graph(np.ascontiguousarray(g.layers, dtype=np.int32).reshape(-1), np.ascontiguousarray(edges[:, 0]),
                    np.ascontiguousarray(edges[:, 1]), as_f32(g.values), as_f32(g.edge_values),
                    None if g.labels is None else [str(s) for s in g.labels])


NetworkView.set_graph = _set_graph


def _set_state(self: CartPendulumView, base, joints, push: float = 0.0) -> None:
    """Main chain: `base` (x, y) on the rail, `joints` the end of each link (N x 2), base outward;
    `push` draws a disturbance arrow at the tip. World units, y down. Thread-safe."""
    j = np.ascontiguousarray(joints, dtype=np.float32).reshape(-1, 2)
    self._set_state(float(base[0]), float(base[1]), j, float(push))


def _set_ghosts(self: CartPendulumView, bases, joints) -> None:
    """Faded chains behind the main one: `bases` (G x 2), `joints` (G x N x 2). Thread-safe."""
    b = np.ascontiguousarray(bases, dtype=np.float32).reshape(-1, 2)
    j = np.ascontiguousarray(joints, dtype=np.float32).reshape(len(b), -1, 2)
    self._set_ghosts(b, j)


CartPendulumView.set_state = _set_state
CartPendulumView.set_ghosts = _set_ghosts


def _push_stats(self: MetricStore, iteration: int, best_score: float = 0.0, sim_time: float = 0.0,
                wall_time: float = 0.0, prefix: str = "", **extra: float) -> None:
    """Pushes training progress as series `<prefix>iteration`, `best_score`, `sim_time`,
    `wall_time` and one per extra keyword, all at step = iteration (see `training_stats`)."""
    step = float(iteration)
    for name, value in [("iteration", step), ("best_score", best_score), ("sim_time", sim_time),
                        ("wall_time", wall_time), *extra.items()]:
        self.push(prefix + name, float(value), step)


MetricStore.push_stats = _push_stats


def mlp_graph(model, x=None, input_labels: Sequence[str] | None = None,
              output_labels: Sequence[str] | None = None, activations: Sequence | None = None) -> LayeredGraph:
    """LayeredGraph of a multilayer perceptron; edge values are the weights.

    `model` is a torch module (its `nn.Linear` layers, in call order) or a list of weight
    matrices shaped (out, in). Node values come from one input sample: with torch, pass `x` and
    each layer shows the output of the module right after its Linear (the activation, if any);
    with weight matrices, pass `activations` (one array per layer, input first)."""
    if isinstance(model, (list, tuple)):
        weights = [np.asarray(w, dtype=np.float32) for w in model]
        acts = None if activations is None else [np.asarray(a, dtype=np.float32).reshape(-1) for a in activations]
    else:
        weights, acts = _torch_mlp(model, x)
    sizes = [weights[0].shape[1]] + [w.shape[0] for w in weights]
    first = np.cumsum([0] + sizes[:-1])
    layers = np.repeat(np.arange(len(sizes)), sizes)
    src, dst, ev = [], [], []
    for l, w in enumerate(weights):
        o, i = np.meshgrid(np.arange(w.shape[0]), np.arange(w.shape[1]), indexing="ij")
        src.append((first[l] + i).ravel())
        dst.append((first[l + 1] + o).ravel())
        ev.append(w.ravel())
    values = None if acts is None else np.concatenate(acts)
    labels = None
    if input_labels is not None or output_labels is not None:
        labels = [""] * int(sum(sizes))
        for k, s in enumerate(input_labels or []):
            labels[k] = s
        for k, s in enumerate(output_labels or []):
            labels[int(first[-1]) + k] = s
    return LayeredGraph(layers, np.stack([np.concatenate(src), np.concatenate(dst)], axis=1), values,
                        np.concatenate(ev), labels)


def _torch_mlp(model, x):
    import torch  # optional dependency

    linears = []
    calls = []  # (module, output) of every leaf module, in call order
    hooks = []
    for mod in model.modules():
        if isinstance(mod, torch.nn.Linear):
            linears.append(mod)
        if not list(mod.children()):
            hooks.append(mod.register_forward_hook(lambda m, i, o: calls.append((m, i[0], o))))
    acts = None
    try:
        if x is not None:
            with torch.no_grad():
                model(torch.as_tensor(x, dtype=torch.float32).reshape(1, -1))
    finally:
        for h in hooks:
            h.remove()
    if x is not None:
        order = [k for k, (m, _, _) in enumerate(calls) if isinstance(m, torch.nn.Linear)]
        linears = [calls[k][0] for k in order]
        acts = [calls[order[0]][1][0].numpy()]
        for n, k in enumerate(order):
            end = order[n + 1] if n + 1 < len(order) else len(calls)
            acts.append(calls[end - 1][2][0].numpy())  # last module before the next Linear
    weights = [m.weight.detach().cpu().numpy() for m in linears]
    return weights, [a.astype(np.float32).reshape(-1) for a in acts] if acts else None
