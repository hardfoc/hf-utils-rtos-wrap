[⬅️ Previous](Queues.md) | [🗂️ Index](index.md) | [➡️ Next](RTOSAbstraction.md)

# Generic Templates

`hf-utils-rtos-wrap` ships three thread-aware generic templates that capture
the patterns the HardFOC codebase needs over and over: a flag bitset, a
single-writer / many-reader snapshot, and a fixed-capacity error ring.

All three follow the **same shape**:

> a pure-virtual **Reader** ABC + a pure-virtual **Writer** ABC + a concrete
> `final` class that derives from both.

Hand a `Reader*` to subscribers that may only inspect or wait, hand a
`Writer*` to producers — the type system then enforces the apps ↔ middleware
layering rules described in [`AGENTS.md`](../../../../../../AGENTS.md).

![Reader / Writer / Saver architecture](assets/generic-template-architecture.svg)

## Table of Contents
1. [`hf::FlagsSaver`](#hfflagssaverflagid-n)
2. [`hf::SeqlockSnapshot`](#hfseqlocksnapshott)
3. [`hf::ErrorHistory`](#hferrorhistoryrecord-n)
4. [Layering note](#layering-note)

---

## `hf::FlagsSaver<FlagId, N>`

Header: [`FlagsSaver.h`](../include/FlagsSaver.h)

Atomic, two-state bitset indexed by a strongly-typed enum. One bit per slot,
packed 64 slots per `std::atomic<uint64_t>` word. Reads and writes are
lock-free; an optional waiter is backed by an `OsEventFlags` so that
subscribers can block until any flag changes.

| Surface | Provided by | Notes |
|---|---|---|
| `IsSet(id)` | `FlagsReader` | Lock-free single-bit load |
| `Snapshot_(out)` | `FlagsReader` | Coherent copy of all words + `seq` + `last_change_ms` |
| `Seq()` | `FlagsReader` | Monotonic counter; bumps on every successful `Set` / `Clear` |
| `LastChangeMs()` | `FlagsReader` | Caller-supplied timestamp from the last write |
| `WaitForChange(timeout_ms)` | `FlagsReader` | Lazy event group; task-context only |
| `Set(id, now_ms)` / `Clear(id, now_ms)` | `FlagsWriter` | Single `fetch_or` / `fetch_and`; bumps `seq` and signals waiter |
| `ClearAll(now_ms)` | `FlagsWriter` | Resets every word atomically |

```cpp
#include "FlagsSaver.h"

enum class SystemFlag : uint8_t { kReady = 0, kFault, kCount };

hf::FlagsSaver<SystemFlag,
               static_cast<std::size_t>(SystemFlag::kCount)> flags;

flags.Set(SystemFlag::kReady, os_get_tick_ms());
if (flags.IsSet(SystemFlag::kReady)) { /* ... */ }
flags.WaitForChange(/*timeout_ms=*/100);
```

**Thread-safety:** lock-free for `Set` / `Clear` / `IsSet` / `Snapshot_` /
`Seq` / `LastChangeMs`. `WaitForChange` is task-context only (FreeRTOS event
group). Not ISR-safe via this path.

**Allocation:** none on the hot path. Storage is a fixed-size atomic word
array sized at compile time; the event group is created lazily on first wait.

---

## `hf::SeqlockSnapshot<T>`

Header: [`SeqlockSnapshot.h`](../include/SeqlockSnapshot.h)

Single-writer / many-reader coherent snapshot of a trivially-copyable POD.
A classic seqlock: the writer increments a 32-bit sequence counter to **odd**
before copying, then back to **even** afterwards. Readers retry if they
witness an odd sequence or if the sequence changed across the read.

![Publish / Read flow](assets/seqlock-flow.svg)

| Surface | Provided by | Notes |
|---|---|---|
| `Read(out)` → `seq` | `SnapshotReader` | Wait-free in the absence of writes; bounded retry under contention |
| `Seq()` | `SnapshotReader` | Even when stable; odd while a `Publish` is in progress |
| `WaitForChange(timeout_ms)` / `ClearWaitEvent()` | `SnapshotReader` | Optional waiter; lazy event group |
| `Publish(value)` | `SnapshotWriter` | Single-writer assumption; protect externally if multi-writer ever required |

```cpp
#include "SeqlockSnapshot.h"

struct ManifoldSnapshot { /* trivially copyable POD */ };

hf::SeqlockSnapshot<ManifoldSnapshot> latest;

// Aggregator (single writer)
latest.Publish(snapshot);

// Anywhere (many readers)
ManifoldSnapshot out;
auto seq = latest.Read(out);
```

**Thread-safety:** single writer for `Publish`, many readers for `Read`
(wait-free in steady state, bounded retry under contention). `WaitForChange`
is task-context only.

**Allocation:** none on the hot path. Inline `T` storage; event group is
created lazily on first `WaitForChange` / `ClearWaitEvent`.

**Constraint:** `static_assert(std::is_trivially_copyable_v<T>)`.

---

## `hf::ErrorHistory<Record, N>`

Header: [`ErrorHistory.h`](../include/ErrorHistory.h)

Fixed-capacity ring buffer of error / event records. All operations take an
internal `RtosMutex`; pushing to a full ring overwrites the oldest entry and
bumps an overwrite counter so callers can detect loss.

| Surface | Provided by | Notes |
|---|---|---|
| `Size()` / `Capacity()` | `ErrorHistoryReader` | Current count and template-fixed capacity |
| `Seq()` | `ErrorHistoryReader` | Monotonic counter; bumps on `Push` / `Pop` / `Clear` |
| `OverwriteCount()` | `ErrorHistoryReader` | Number of `Push` calls that overwrote an older entry |
| `Snapshot(out, max)` | `ErrorHistoryReader` | Copies up to `max` records oldest → newest |
| `Push(record)` | `ErrorHistoryWriter` | `false` if the mutex cannot be acquired; not ISR-safe |
| `Pop(out)` | `ErrorHistoryWriter` | Removes and returns the oldest record |
| `Clear()` | `ErrorHistoryWriter` | Drops every entry, resets size, bumps `seq` |

```cpp
#include "ErrorHistory.h"

struct ErrorRecord { uint32_t ts_ms; uint16_t code; char src[16]; };

hf::ErrorHistory<ErrorRecord, /*Capacity=*/32> history;

history.Push(ErrorRecord{ /* ... */ });

ErrorRecord buf[32];
auto n = history.Snapshot(buf, 32);
```

**Thread-safety:** all operations under `RtosMutex`; safe from any task
context. **Not ISR-safe** — buffer ISR-side records to an `OsQueue` and drain
from a task.

**Allocation:** none. Backing storage is an inline `Record[N]`.

**Constraint:** `static_assert(std::is_trivially_copyable_v<Record>)` and
`N > 0`.

---

## Layering note

The three Reader/Writer ABCs let middleware-side and apps-side code coexist
on the same `Saver` instance without leaking apps-tier types into middleware
headers. A typical pattern in this codebase looks like:

```cpp
// middleware/api/MwApi.h
class MwApi {
public:
    [[nodiscard]] hf::FlagsReader<MwSystemFlagId, kMwSystemFlagCount>&
    SystemFlagsReader() noexcept;          // read-only handle for downstream

private:
    hf::FlagsSaver<MwSystemFlagId, kMwSystemFlagCount> system_flags_;  // owner
};
```

Apps that need to mutate flags receive `FlagsWriter*`; those that only
display them receive `FlagsReader*`. The pattern repeats verbatim for
`SnapshotReader`/`Writer` and `ErrorHistoryReader`/`Writer`.

[⬅️ Previous](Queues.md) | [🗂️ Index](index.md) | [➡️ Next](RTOSAbstraction.md)
