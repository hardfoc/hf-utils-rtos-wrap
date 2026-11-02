[⬅️ Previous](BaseThread.md) | [🗂️ Index](index.md) | [➡️ Next](Queues.md)

# Synchronization Primitives

`hf-utils-rtos-wrap` provides a small set of synchronisation helpers. None of
them perform lazy allocation: the underlying RTOS handle is created in the
constructor (or, for `OsEventFlags` / `FlagsSaver`'s waiter, on first use of
the wait path).

## Highlights

- **`RtosMutex`** — `std::lock_guard`-compatible. Use this for most new code.
- **`Mutex` + `MutexGuard`** — named-mutex pair consumed by
  `BaseThread` infrastructure.
- **`SignalSemaphore`** — named binary semaphore for start/stop / wake events.
- **`OsEventFlags`** — event-group wrapper with strongly-typed `WaitMode`.
- **`CriticalGuard`** — RAII helper that wraps `portENTER_CRITICAL` /
  `portEXIT_CRITICAL` for short ISR-disabled sections.

## `RtosMutex`

The authoritative mutex for HardFOC handlers, managers, and API code.
Compatible with `std::lock_guard<RtosMutex>` and `std::unique_lock<RtosMutex>`.

```cpp
#include "RtosMutex.h"
#include <mutex>

RtosMutex m;
{
    std::lock_guard<RtosMutex> lock(m);
    // critical section
}
```

## `Mutex` + `MutexGuard`

Named-mutex pair. `MutexGuard` locks in its constructor and unlocks on destruction;
it accepts a timeout for the lock attempt.

```cpp
#include "Mutex.h"
#include "MutexGuard.h"

Mutex m;
{
    MutexGuard lock(&m);   // blocks forever
    // critical section
}
```

## `SignalSemaphore`

A thin C++ wrapper around a FreeRTOS binary semaphore — the building block
behind `BaseThread`'s start / stop verification.

## `OsEventFlags` (modernized)

```cpp
#include "OsEventFlags.h"

OsEventFlags<> flags;
flags.Set(0x3);

uint32_t got = 0;
if (flags.Wait(/*bits=*/0x3, WaitMode::All, /*timeout_ms=*/50, got)) {
    // both bits arrived
}
```

`Wait` takes a strongly-typed `enum class WaitMode { Any, All }`; pass
`UINT32_MAX` for an infinite timeout. The previous `OS_OR` / `OS_AND` /
`OS_Ulong` API has been removed.

## `CriticalGuard`

```cpp
{
    CriticalGuard irqLock;   // interrupts disabled
    // very short critical section
}
```

Use sparingly — only for sequences too short for a mutex.

## See also

- [`docs/GenericTemplates.md`](GenericTemplates.md) — the higher-level
  `hf::FlagsSaver`, `hf::SeqlockSnapshot`, and `hf::ErrorHistory` templates
  composed on top of these primitives.

[⬅️ Previous](BaseThread.md) | [🗂️ Index](index.md) | [➡️ Next](Queues.md)
