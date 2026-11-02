# Queues

`OsQueue<T, N>`: a typed, fixed-capacity queue for passing values between tasks, with millisecond timeouts.

## Purpose

Hand data from a producer task to a consumer task without a lock of your own. Values are copied in and out, so
`T` must be trivially copyable; the kernel queue is already safe for many producers and many consumers.

## Example

```cpp
--8<-- "tests/docs/queues.cpp:queue"
```

## API

| Member | Does | Returns |
| --- | --- | --- |
| `OsQueue(const char* name)` | Creates the kernel queue | Check `IsValid()` |
| `Send(const T&, timeout_ms = UINT32_MAX)` | Copies the value in, waiting for space up to `timeout_ms` | False on timeout (queue full) |
| `Receive(T&, timeout_ms = UINT32_MAX)` | Copies the oldest value out, waiting up to `timeout_ms` | False on timeout (queue empty) |
| `IsValid()` | | True when the kernel queue exists |
| `Capacity()` | `static constexpr` | `N` |

`timeout_ms = 0` does not wait. The queue is not copyable.

## Memory and timing

- **Construction is eager.** The kernel queue is created in the constructor; check `IsValid()` once at start-up.
- **The queue lives in the object.** On FreeRTOS the queue is created with `xQueueCreateStatic()` in the object's
  inline storage (the kernel's `StaticQueue_t` followed by `N` items), so nothing comes from the kernel heap. On
  ThreadX the items live there too. A FreeRTOS build without `configSUPPORT_STATIC_ALLOCATION` falls back to
  `xQueueCreate()`, and CMSIS-RTOS2 allocates its queues in the kernel.
- **It holds exactly `N` items.** `Send()` with `timeout_ms = 0` fails on the `N + 1`th item until one is received.
- **Items are copied in 32-bit words.** The kernel stores `sizeof(T)` rounded up to a multiple of 4 bytes. `Send()`
  and `Receive()` copy through a word buffer, so a `T` of any size is read and written only within its own bytes.
- **Send and receive are O(1)** plus the copy of one item.
- **`T` must be trivially copyable**, which a `static_assert` checks.

On the no-RTOS backend `Send()` returns true and stores nothing, and `Receive()` returns false.

Next: [Timers](timers.md).
