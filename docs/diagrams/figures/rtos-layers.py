"""The layers of the RTOS wrapper: C++ classes, the C portability layer and one backend per scheduler.

Reads: From top to bottom. Your code uses the C++ classes (listed from include/*.h). Every class calls the C
portability layer, the OS_* types and os_* functions of OsAbstraction.h and OsUtility.h, and nothing else. That
layer includes exactly one backend header from include/os/, chosen by a compile definition (read from the table
in OsAbstraction.h); with no definition the no-RTOS backend is used, for host tests and bare metal. The backend
is the only code that names a kernel.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
ABSTRACTION = "include/OsAbstraction.h"
KERNEL = {
    "os_freertos.h": "FreeRTOS 11.1 (ESP-IDF SMP or vanilla)",
    "os_threadx.h": "Azure RTOS ThreadX",
    "os_cmsis.h": "CMSIS-RTOS2 (osThreadNew, osDelay)",
    "os_none.h": "no scheduler: host tests, bare metal",
}


def data():
    classes = []
    for path in sorted((ROOT / "include").glob("*.h")):
        text = path.read_text()
        for m in re.finditer(r"^(?:template\s*<[^>]*>\s*)?class\s+(\w+)(?:\s+final)?\s*(?::[^{;]*)?\{", text, re.M):
            name = m.group(1)
            if name not in classes:
                classes.append(name)
    rows = re.findall(r"^ \* \| (`HF_RTOS_\w+`|\*\(none\)\*)\s*\|[^|]*\|\s*`os/(os_\w+\.h)`", (ROOT / ABSTRACTION).read_text(),
                      re.M)
    backends = [{"macro": macro.strip("`*").replace("(none)", "no definition"), "header": header}
                for macro, header in rows]
    return {
        "classes": classes,
        "backends": backends,
        "sources": ["include/*.h:classes", f"{ABSTRACTION}:backend table"],
    }


def build(d):
    f = Fig(
        "rtos-layers",
        "One API, one backend per scheduler",
        alt="Layer diagram: your code uses the C++ wrapper classes, which call the C portability layer of "
        "OsAbstraction.h and OsUtility.h, which includes one backend header chosen at build time: FreeRTOS, "
        "ThreadX, CMSIS-RTOS2, or the no-RTOS backend for host tests.",
        width=1200,
        kicker="Architecture",
        subtitle="Classes never name a kernel. A compile definition picks the backend; with none, the host backend.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "one backend, chosen at build time"), ("slate", "yours")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your code", ["threads, handlers, managers, tests"], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, d["classes"], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "C++ wrappers", tone="brand",
                   sub="include/*.h")
    c = f.card(0, reg.b + 40, W, "C portability layer",
               ["OsAbstraction.h, OsUtility.h: OS_* types and os_* functions (threads, queues, mutexes,",
                "semaphores, event groups, timers, critical sections, time)"], tone="brand", accent=True)
    n = len(d["backends"])
    gap = 20
    bw = (W - gap * (n - 1)) / n
    y = c.b + 40
    cards = []
    for i, b in enumerate(d["backends"]):
        cards.append(f.card(i * (bw + gap), y, bw, b["header"], [b["macro"], KERNEL.get(b["header"], "")],
                            tone="teal", accent=True))
    for cb in cards:
        f.connect(c, cb, "bottom", "top", bt=0.5, tone="rail")
    f.connect(app, reg, "bottom", "top", tone="rail")
    f.connect(reg, c, "bottom", "top", tone="rail")
    return f
