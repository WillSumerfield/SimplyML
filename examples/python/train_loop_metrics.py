"""Fake training loop in plain Python, with a live dashboard on its own thread.
Press Q or close the window to stop early."""

# %%
import math
import random
import time

import simplyml

app = simplyml.App(title="train_loop_metrics", size=(1600, 900))

root = app.ui.row()
side = root.column(size=340)
side.stat_tile("Epoch", "epoch", color="teal", fmt="03d")
card = side.stat_card("Run", color="blue")
card.big("Best val loss", "best_val", fmt=".4")
card.gauge("Learning rate", "lr", lo=0, hi=1e-3, color="yellow", fmt=".6")
card.gauge("Progress", "progress", color="accent", fmt=".1%")
card.value("Elapsed", "elapsed", fmt="duration")
card.status("Validation", "validating", on="Running", off="Idle")

grid = root.grid(columns=2)
grid.line_chart("Loss", series=["train_loss", "val_loss"], colors=["accent", "blue"], labels=["train", "val"], span=2, id="loss")
grid.line_chart("Accuracy", series="acc", color="teal", y_range=(0, 1), fmt=".1%")
grid.bar_chart("Val accuracy / epoch", "val_acc", color="green", window=30, fmt=".3")

# %%
epochs, steps_per_epoch = 60, 100
start = time.monotonic()
best = math.inf
with app:
    app.store.push("validating", 0)
    for epoch in range(epochs):
        lr = 1e-3 * 0.95**epoch
        app.store.push("epoch", epoch)
        app.store.push("lr", lr)
        for i in range(steps_per_epoch):
            step = epoch * steps_per_epoch + i
            loss = 2.0 * math.exp(-step / 1500) + 0.05 + 0.04 * random.gauss(0, 1)
            app.store.push("train_loss", loss, step=step)
            app.store.push("acc", min(1.0, 1 - 0.9 * math.exp(-step / 1200) + 0.01 * random.gauss(0, 1)), step=step)
            time.sleep(0.002)  # "work"
        app.store.push("validating", 1)
        val = 2.1 * math.exp(-(epoch + 1) * steps_per_epoch / 1700) + 0.09 + 0.02 * random.gauss(0, 1)
        best = min(best, val)
        app.store.push("val_loss", val, step=(epoch + 1) * steps_per_epoch)
        app.store.push("val_acc", 1 - 0.85 * math.exp(-epoch / 12), step=epoch)
        app.store.push("best_val", best)
        app.store.push("progress", (epoch + 1) / epochs)
        app.store.push("elapsed", time.monotonic() - start)
        app.store.push("validating", 0)

        if any(e.key == "q" or e.type == "closed" for e in app.poll_events()) or not app.is_running:
            break
    app["loss"].title = "Loss (done)"
    app.wait()  # keep the window open until it is closed
