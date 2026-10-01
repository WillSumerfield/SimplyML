# Install

SimplyML runs on **Linux and Windows**. macOS isn't supported, since it won't let a window live off the main thread. Python needs **3.10 or newer**.

## Python wheels

Prebuilt wheels are attached to each [GitHub Release](https://github.com/WillSumerfield/SimplyML/releases). They're not on PyPI, so you install straight from the URL. Pick your platform and Python:

| Platform | Python 3.10 | Python 3.11 | Python 3.12+ |
|---|---|---|---|
| Linux x86_64 | [cp310](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp310-cp310-manylinux_2_28_x86_64.whl) | [cp311](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp311-cp311-manylinux_2_28_x86_64.whl) | [abi3](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp312-abi3-manylinux_2_28_x86_64.whl) |
| Windows x64 | [cp310](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp310-cp310-win_amd64.whl) | [cp311](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp311-cp311-win_amd64.whl) | [abi3](https://github.com/WillSumerfield/SimplyML/releases/download/v0.1.0/simplyml-0.1.0-cp312-abi3-win_amd64.whl) |

```sh
uv pip install <wheel url>
```

Want `mlp_graph` to read torch models? Add the torch extra:

```sh
uv pip install "simplyml[torch] @ <wheel url>"
```

Wheels need no compiler or extra packages. On Linux they use your system's X11 and OpenGL, which any desktop already has.

## Python from source

If there's no wheel for you, or you're changing SimplyML itself, build it. On Linux you'll need SFML's system packages first:

```sh
sudo apt install libx11-dev libxrandr-dev libxcursor-dev libxi-dev libgl1-mesa-dev libudev-dev
# Fedora: sudo dnf install libX11-devel libXrandr-devel libXcursor-devel libXi-devel mesa-libGL-devel systemd-devel
```

On Windows you just need MSVC. Then, from a clone:

```sh
uv venv --python 3.12
uv pip install -e .     # the first build takes about a minute
```

After changing C++ code, run `uv pip install -e .` again to rebuild.

## C++

You need CMake 3.24+, a C++17 compiler and, on Linux, the same system packages as above. SFML 3 is fetched and built for you, so you don't need it installed.

The easiest route is FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(SimplyML
    GIT_REPOSITORY https://github.com/WillSumerfield/SimplyML.git
    GIT_TAG        v0.1.0)
FetchContent_MakeAvailable(SimplyML)

target_link_libraries(my_app PRIVATE SimplyML::SimplyML)
```

Or install it once and use `find_package`:

```sh
cmake -S . -B build && cmake --build build -j
cmake --install build --prefix ~/.local
```

```cmake
find_package(SimplyML REQUIRED)
target_link_libraries(my_app PRIVATE SimplyML::SimplyML)
```

`SimplyML::SimplyML` pulls in everything. If you only need part of it, link a single layer instead:

- `SimplyML::util`: small math and formatting helpers, header-only, no SFML.
- `SimplyML::core`: the App, window, stores and events.
- `SimplyML::ui`: layout, widgets and controls.
- `SimplyML::ml`: network view, cart-pendulum and training stats.
