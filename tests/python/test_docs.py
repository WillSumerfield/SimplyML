"""Runs every ```python block in README.md and docs/*.md, each in a fresh interpreter. A block
preceded by `<!-- no-test -->` is skipped."""

import os
import re
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
BLOCK = re.compile(r"(<!-- no-test -->\s*)?```python\n(.*?)```", re.S)


def snippets():
    for path in [ROOT / "README.md", *sorted((ROOT / "docs").glob("*.md"))]:
        text = path.read_text(encoding="utf-8")
        for m in BLOCK.finditer(text):
            line = text.count("\n", 0, m.start(2)) + 1
            marks = [pytest.mark.skip(reason="no-test")] if m.group(1) else []
            yield pytest.param(m.group(2), id=f"{path.relative_to(ROOT).as_posix()}:{line}", marks=marks)


@pytest.mark.skipif(sys.platform != "win32" and not os.environ.get("DISPLAY"), reason="needs a display")
@pytest.mark.parametrize("code", list(snippets()))
def test_snippet(code, tmp_path):
    r = subprocess.run([sys.executable, "-c", code], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert r.returncode == 0, r.stderr
