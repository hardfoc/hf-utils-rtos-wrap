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
- **Items are copied in 32-bit words.** The item size passed to the kernel is `sizeof(T)` rounded up to a multiple
  of 4 bytes.
- **Send and receive are O(1)** plus the copy of one item. `Send()` copies the value to a local first.

!!! warning "Use a `T` whose size is a multiple of 4 bytes on FreeRTOS"
    The FreeRTOS backend creates the queue with `xQueueCreate()` on the kernel heap; the inline `storage_` array
    is not used, and the queue length passed is `sizeof(storage_)` (bytes), not `N`. Because each item is rounded
    up to whole words, the kernel copies more than `sizeof(T)` bytes into and out of a `T` whose size is not a
    multiple of 4. Pad small types (wrap a `uint8_t` in a struct with a `uint32_t`) until the backend is fixed.

On the no-RTOS backend `Send()` returns true and stores nothing, and `Receive()` returns false.

Next: [Timers](timers.md).
