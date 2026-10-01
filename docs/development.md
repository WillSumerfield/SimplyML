# Development

How to build, test and release SimplyML itself. You'll need the source build setup from [install](install.md#python-from-source) (the Linux system packages, CMake 3.24+, a C++17 compiler, and uv for Python).

## Build and test

C++:

```sh
cmake -S . -B build          # Release by default; fetches SFML 3 and doctest
cmake --build build -j
ctest --test-dir build       # unit tests, plus a find_package install check
```

Python:

```sh
uv venv --python 3.12
uv pip install -e ".[test]"
uv run pytest
```

The Python build lives in `build/py-<tag>/`, so SFML isn't rebuilt every time you reinstall. After changing C++ code, run `uv pip install -e .` again.

The window tests need a display. On Linux without one (CI, SSH), use `xvfb-run -a ctest ...` or start `Xvfb` and set `DISPLAY`. Some Python tests send real key presses through X, so they only run on Linux. `tests/python/test_docs.py` runs every Python snippet in these docs. Put `<!-- no-test -->` right before a snippet that can't run on its own.

## Repo layout

```
include/simplyml/   public C++ headers, one folder per layer
src/                C++ sources, plus src/python/ for the bindings
python/simplyml/    the Python package (pure-Python helpers on top of the bindings)
tests/              C++ tests (doctest, via ctest) and tests/python/ (pytest)
examples/           C++ and Python examples, and pendulum_neat
docs/               these docs
res/                the embedded font
```

The library is split into layers, and each one only uses the ones before it: `util` → `core` → `ui` → `ml`, with `python` on top. CMake enforces this, since each layer is its own target.

Code style: everything is in namespace `sml`, files are snake_case, types are PascalCase, functions are camelCase and members start with `m_`. MSVC has to build it too, so avoid GNU-only extensions.

If you work with a coding agent, `CONTEXT-MAP.md` and the `CONTEXT.md` glossary in each `src/<layer>/` are written for it.

## Releasing

Wheels are built in CI and attached to a GitHub Release. Nothing goes to PyPI.

1. Bump `version` in `pyproject.toml`, and update the wheel links in the root `README.md` and `docs/install.md` to match.
2. Commit and push.
3. Run the **release** workflow from the Actions tab (or `gh workflow run release.yml`).

The workflow runs the C++ tests, then builds and tests wheels for Linux and Windows (Python 3.10, 3.11, and one wheel for 3.12+), builds the sdist, and creates Release `v<version>` with all of them attached. It refuses to run if that version's tag already exists.
