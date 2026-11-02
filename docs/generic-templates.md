# Generic templates

`hf::FlagsSaver`, `hf::SeqlockSnapshot` and `hf::ErrorHistory`: shared state for status flags, the latest value
of a struct and a history of errors, each with a read side and a write side.

## The shape they share

Each template is three types in one header: a pure-virtual reader interface, a pure-virtual writer interface and a
`final` class that implements both. Give producers a reference to the writer and consumers a reference to the
reader; the type system then stops a consumer from writing.

| Template | Reader | Writer | Concurrency | Storage |
| --- | --- | --- | --- | --- |
| `hf::FlagsSaver<Id, N>` | `hf::FlagsReader<Id, N>` | `hf::FlagsWriter<Id, N>` | Lock-free, any number of tasks | `(N + 63) / 64` atomic 64-bit words |
| `hf::SeqlockSnapshot<T>` | `hf::SnapshotReader<T>` | `hf::SnapshotWriter<T>` | One writer, any number of readers, no locks | One `T` and a sequence counter |
| `hf::ErrorHistory<R, N>` | `hf::ErrorHistoryReader<R>` | `hf::ErrorHistoryWriter<R>` | Internal `RtosMutex` | `R[N]` ring |

None allocates on the heap. `T` and `R` must be trivially copyable (a `static_assert` checks it). None is for
interrupt context.

## FlagsSaver

A set of on / off flags named by an enum. `Set()` and `Clear()` are one atomic operation each, return false when
the flag was already in that state, and bump a sequence counter. A waiter can block until any flag changes.

```cpp
--8<-- "tests/docs/generic.cpp:flags"
```

| Member | Side | Does |
| --- | --- | --- |
| `Set(id, now_ms = 0)` / `Clear(id, now_ms = 0)` | Writer | Change one flag; false when it already had that state |
| `ClearAll(now_ms = 0)` | Writer | Clear every flag |
| `IsSet(id)` | Reader | One atomic load |
| `Snapshot_(out)` | Reader | Copy of all words, the sequence number and the last change time |
| `Seq()`, `LastChangeMs()` | Reader | Change counter; the `now_ms` of the last change |
| `WaitForChange(timeout)` / `ClearWaitEvent()` | Reader | Block until a change is signalled; discard a pending signal |

`WaitForChange()` consumes the change signal when it returns true, so the next call waits for a new change.
Call `ClearWaitEvent()` to forget changes made before you start waiting, as the example does at the top. The
timeout is milliseconds, converted to kernel ticks like every other timeout in the library.

The event group behind the waiter is created on the first `WaitForChange()` or `ClearWaitEvent()`.

## SeqlockSnapshot

The latest value of a struct, published by one task and read by any number, without locks. The writer makes the
sequence counter odd, copies the value and makes it even again; a reader copies the value and retries if the
counter was odd or changed meanwhile.

![Seqlock flow: the writer increments the sequence to odd, copies the payload and increments it to even;
the reader reads the sequence, copies the payload, reads it again and retries on a
mismatch](assets/seqlock-flow.svg){ .hf-plate }

```cpp
--8<-- "tests/docs/generic.cpp:snapshot"
```

| Member | Side | Does |
| --- | --- | --- |
| `Publish(value)` | Writer | Copy `value` in; one writer only |
| `Read(out)` | Reader | Copy a consistent value out; returns the (even) sequence number it read |
| `Seq()` | Reader | Current sequence: even when stable, odd during a publish |
| `WaitForChange(timeout)` / `ClearWaitEvent()` | Reader | As for `FlagsSaver` |

`Read()` costs two atomic loads and one `memcpy` of `T` when no publish is in progress, and retries while one
is. Two tasks calling `Publish()` at once corrupt the value: protect them with a mutex if you ever have two
writers.

## ErrorHistory

The last `N` records of something (errors, events), oldest first. A full ring overwrites its oldest record and
counts the overwrite, so a reader can tell that records were lost.

```cpp
--8<-- "tests/docs/generic.cpp:history"
```

| Member | Side | Does |
| --- | --- | --- |
| `Push(record)` | Writer | Append; overwrites the oldest when full; false if the mutex could not be taken |
| `Pop(out)` | Writer | Remove the oldest |
| `Clear()` | Writer | Remove all |
| `Size()`, `Capacity()` | Reader | Records held; `N` |
| `Snapshot(out, max)` | Reader | Copy up to `max` records, oldest first; returns the count |
| `Seq()`, `OverwriteCount()` | Reader | Change counter; records lost to overwrites |

Every call takes the internal `RtosMutex`, so all are O(1) except `Snapshot()`, which is O(`max`).

Next: [Utility](utility.md).
