"""``<!-- generated: quickstart-code -->``: the Quickstart example, copied from the compiled snippet file.

The README cannot include files the way the site does, so this region carries a copy of the ``thread`` and
``start`` sections of ``tests/docs/base_thread.cpp`` (compiled by ``tools/docs/check-snippets.sh`` in CI).
``hardfoc-docs gen --check`` fails when the copy and the compiled file differ.
"""

from __future__ import annotations

import re

from hardfoc_mkdocs_theme.paths import repo_root


def _section(path: str, name: str) -> str:
    text = (repo_root() / path).read_text()
    m = re.search(rf"--8<-- \[start:{name}\]\n(.*?)\n[^\n]*--8<-- \[end:{name}\]", text, re.S)
    if not m:
        raise SystemExit(f"[gen] no section {name!r} in {path}")
    return m.group(1)


def quickstart_code() -> str:
    body = _section("tests/docs/base_thread.cpp", "thread") + "\n\n" + _section("tests/docs/base_thread.cpp", "start")
    return "```cpp\n" + body + "\n```\n"


REGIONS = {"quickstart-code": quickstart_code}
