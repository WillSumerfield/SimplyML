"""A small numpy MLP fitting a sine wave, steered live from the dashboard: drag the learning rate,
switch optimizer, change the batch size, pause (Space) or reset (R). The training loop only reads
`app.controls`; the window runs on its own thread."""

# %%
import time

import numpy as np

import simplyml

app = simplyml.App(title="controls", size=(1600, 900))

root = app.ui.row()
side = root.column(size=380)
panel = side.control_panel("Training", color="accent")
panel.toggle("pause", "Paused", key="space")
panel.slider("lr", "Learning rate", lo=1e-4, hi=1.0, value=3e-2, log=True)
panel.select("optimizer", ["SGD", "Momentum", "Adam"], label="Optimizer", value="Adam", key="o")
panel.number("batch", "Batch size", value=32, lo=1, hi=1024, integer=True)
panel.slider("hidden", "Hidden units (on reset)", lo=4, hi=128, value=32, step=4)
panel.button("reset", "Reset", color="accent", key="r")
side.key_bindings()
app.bind_key("escape", "Quit", app.close)

charts = root.column()
charts.line_chart("Loss", series=["train_loss", "val_loss"], colors=["accent", "blue"], labels=["train", "val"],
                  fmt=".4", weight=2)
row = charts.row()
row.line_chart("Learning rate", series="lr", color="yellow", fmt=".2e", include_zero=False)
card = row.stat_card("Run", color="teal")
card.big("Step", "step")
card.value("Best val loss", "best", fmt=".5")

# %%
rng = np.random.default_rng(0)
x_train = rng.uniform(-np.pi, np.pi, (2048, 1))
y_train = np.sin(2 * x_train) + 0.1 * rng.normal(size=x_train.shape)
x_val = np.linspace(-np.pi, np.pi, 256)[:, None]
y_val = np.sin(2 * x_val)


def init(hidden):
    params = [rng.normal(0, 1, (1, hidden)), np.zeros(hidden), rng.normal(0, 1 / np.sqrt(hidden), (hidden, 1)), np.zeros(1)]
    return params, [np.zeros_like(p) for p in params], [np.zeros_like(p) for p in params]


def forward(params, x):
    w1, b1, w2, b2 = params
    h = np.tanh(x @ w1 + b1)
    return h, h @ w2 + b2


def grads(params, x, y):
    w1, b1, w2, b2 = params
    h, out = forward(params, x)
    d_out = 2 * (out - y) / len(x)
    d_h = (d_out @ w2.T) * (1 - h**2)
    return float(np.mean((out - y) ** 2)), [x.T @ d_h, d_h.sum(0), h.T @ d_out, d_out.sum(0)]


params, m, v = init(32)
step, best, last_reset = 0, np.inf, 0
with app:
    while app.is_running:
        c = app.controls
        if c["reset"] != last_reset:
            last_reset = c["reset"]
            params, m, v = init(int(c["hidden"]))
            step, best = 0, np.inf
            app.store.clear()
        if c["pause"]:
            time.sleep(0.02)
            continue

        lr, opt, batch = c["lr"], app["optimizer"].value, int(c["batch"])
        idx = rng.integers(0, len(x_train), batch)
        loss, g = grads(params, x_train[idx], y_train[idx])
        for p, gi, mi, vi in zip(params, g, m, v):
            if opt == "SGD":
                p -= lr * gi
            elif opt == "Momentum":
                mi[:] = 0.9 * mi + gi
                p -= lr * mi
            else:  # Adam
                mi[:] = 0.9 * mi + 0.1 * gi
                vi[:] = 0.999 * vi + 0.001 * gi**2
                p -= lr * (mi / (1 - 0.9 ** (step + 1))) / (np.sqrt(vi / (1 - 0.999 ** (step + 1))) + 1e-8)

        app.store.push("train_loss", loss, step)
        app.store.push("lr", lr, step)
        app.store.push("step", step, step)
        if step % 20 == 0:
            val = float(np.mean((forward(params, x_val)[1] - y_val) ** 2))
            best = min(best, val)
            app.store.push("val_loss", val, step)
            app.store.push("best", best, step)
        step += 1
        time.sleep(0.002)  # keep the demo watchable
