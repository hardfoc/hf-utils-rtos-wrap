[⬅️ Previous](RTOSAbstraction.md) | [🗂️ Index](index.md) | [➡️ Next](Utility.md)

# Periodic Timers

`PeriodicTimer` provides a lightweight wrapper around ESP‑IDF timers. It lets you
run a callback at a fixed interval without dealing with the FreeRTOS API
directly.

## Highlights
- RAII style wrapper that automatically cleans up.
- Uses `OsUtility` so it can be ported to other RTOSes.
- Simple start/stop methods and optional auto-start on creation.

## Quick Example
```cpp
#include "PeriodicTimer.h"

void Blink(uint32_t /*arg*/) {
    // toggle an LED
}

PeriodicTimer blinkTimer;

void app_main() {
    blinkTimer.Create("Blink", Blink, 0, 500, true); // 500 ms period
}
```

The timer will call `Blink` every 500 milliseconds until stopped or destroyed.

[⬅️ Previous](RTOSAbstraction.md) | [🗂️ Index](index.md) | [➡️ Next](Utility.md)
