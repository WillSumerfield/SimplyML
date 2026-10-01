"""Phase-3 smoke example: a Python loop pushes fake metrics while the window runs on its own
thread; key presses come back through poll_events(). Press Q or close the window to stop."""

# %%
import math
import time

import simplyml

with simplyml.App(title="hello simplyml", size=(800, 450)) as app:
    step = 0
    while app.is_running:
        app.store.push("loss", math.exp(-step / 500) + 0.05 * math.sin(step / 7))
        step += 1
        for e in app.poll_events():
            print(e)
            if e.key == "q" or e.type == "closed":
                app.close()
        time.sleep(0.001)
    app.store.write_csv("hello_metrics.csv")
    print(f"pushed {step} points")
