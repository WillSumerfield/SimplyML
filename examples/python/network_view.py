"""A numpy MLP (2-8-8-1, tanh) learning which points lie inside a ring, drawn live: node fill =
activations for a probe input circling the ring, edge width/color = weights. With torch installed,
`simplyml.mlp_graph(model, x=...)` does the same for an nn.Module. Below it, a PlacedGraph: a small
recurrent reservoir (nodes on a ring, self-loops included) driven by the same probe."""

# %%
import time

import numpy as np

import simplyml

app = simplyml.App(title="network view", size=(1600, 900))
root = app.ui.row()
left = root.column(size=440)
card = left.training_stats("Epoch", color="teal", score_label="Best loss")
card.gauge("Accuracy", "acc", 0, 1, color="green", fmt="%")
net = left.network_view("Network", color="white", edge_scale=1.5, max_zoom=2.0)
res = left.network_view("Reservoir", color="white", edge_scale=1.5, max_zoom=2.0)
charts = root.column()
charts.line_chart("Loss", series="loss", color="accent")
charts.line_chart("Accuracy", series="acc", color="green", y_range=(0, 1))

# %%
rng = np.random.default_rng(0)
sizes = [2, 8, 8, 1]
weights = [rng.normal(0, 1 / np.sqrt(i), (o, i)) for i, o in zip(sizes, sizes[1:])]
biases = [np.zeros(o) for o in sizes[1:]]


# Reservoir: input left, output right, N recurrent nodes on a ring between them (y down).
N = 12
angles = 2 * np.pi * np.arange(N) / N
positions = np.vstack([[-2.0, 0.0], np.c_[np.cos(angles), np.sin(angles)], [2.0, 0.0]])
rec = [(i, j) for i in range(N) for j in rng.choice(N, 3, replace=False)]  # j == i makes a self-loop
w_rec = rng.normal(0, 0.6, len(rec))
w_in, w_out = rng.normal(0, 1.0, N), rng.normal(0, 0.5, N)
res_edges = ([(0, 1 + i) for i in range(N)] + [(1 + i, 1 + j) for i, j in rec]
             + [(1 + i, N + 1) for i in range(N)])
res_edge_values = np.concatenate([w_in, w_rec, w_out])
W = np.zeros((N, N))
for (i, j), w in zip(rec, w_rec):
    W[j, i] += w
state = np.zeros(N)


def forward(x):
    acts = [x]
    for w, b in zip(weights, biases):
        acts.append(np.tanh(acts[-1] @ w.T + b))
    return acts


def label(x):
    r = np.hypot(x[:, 0], x[:, 1])
    return np.where((r > 0.35) & (r < 0.75), 1.0, -1.0)[:, None]


start, best = time.monotonic(), np.inf
with app:
    for epoch in range(100_000):
        if not app.is_running:
            break
        x = rng.uniform(-1, 1, (256, 2))
        y = label(x)
        acts = forward(x)
        out = acts[-1]
        loss = float(np.mean(0.5 * (out - y) ** 2))
        delta = (out - y) * (1 - out**2) / len(x)
        for l in reversed(range(len(weights))):
            grad_w, grad_b = delta.T @ acts[l], delta.sum(0)
            delta = (delta @ weights[l]) * (1 - acts[l] ** 2)
            weights[l] -= 2.0 * grad_w
            biases[l] -= 2.0 * grad_b

        best = min(best, loss)
        app.store.push("loss", loss, epoch)
        app.store.push("acc", float(np.mean((out > 0) == (y > 0))), epoch)
        app.store.push_stats(epoch, best_score=best, sim_time=epoch * 2.56, wall_time=time.monotonic() - start)

        t = epoch * 0.05
        probe = np.array([[0.55 * np.cos(t), 0.55 * np.sin(t)]])
        net.set_graph(simplyml.mlp_graph(weights, activations=[a[0] for a in forward(probe)],
                                         input_labels=["x", "y"], output_labels=["inside"]))
        u = float(probe[0, 0])
        state = np.tanh(W @ state + w_in * u)
        res.set_graph(positions=positions, edges=res_edges, edge_values=res_edge_values,
                      values=np.concatenate([[u], state, [np.tanh(w_out @ state)]]),
                      labels=["in"] + [""] * N + ["out"])
        time.sleep(0.01)
