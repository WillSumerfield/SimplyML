# SimplyML

A small library for live ML-training dashboards. <br>
Modified and expanded upon code originally written by Jean Tampon, in [Pendulum-NEAT](https://github.com/johnBuffer/Pendulum-NEAT). <br>
All credit goes to him, all complaints go to me. 

## Quick start

Grab the wheel for your Python from the [install guide](docs/install.md) (Linux or Windows, Python 3.10+). For Python 3.12+ on Linux, that's:

```sh
uv pip install https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp312-abi3-manylinux_2_28_x86_64.whl
```

Then push numbers from your training loop and watch them come in:

```python
import math
import time

import simplyml

app = simplyml.App(title="my run")
app.ui.line_chart("Loss", series="loss")

with app:  # the window runs on its own thread, so your loop never waits on it
    for step in range(2000):
        loss = math.exp(-step / 400)  # your training step goes here
        app.store.push("loss", loss)
        time.sleep(0.001)
```

Using C++? See [install](docs/install.md#c) for CMake setup.

## Docs

Everything else lives in [docs/](docs/README.md): layouts, widgets, controls you can tweak mid-training, network drawings and the examples.

## Credits

SimplyML is extracted from [Pendulum-NEAT](https://github.com/johnBuffer/Pendulum-NEAT) by Jean Tampon (johnBuffer), MIT licensed.

The default font is [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) by Carrois Type Design, under the SIL Open Font License 1.1 (`res/fonts/OFL.txt`).

## License

MIT; see `LICENSE`.
