# Synchronization

Locks, wake-ups, event flags and critical sections: which one to use, a compiled example of each, and what each
costs.

| Class | Use it to | Blocks | Kernel object |
| --- | --- | --- | --- |
| `RtosMutex` | Protect shared data, with `std::lock_guard` | Yes | Recursive mutex, created in the constructor |
| `RtosSharedMutex` | Many readers, one writer | Yes | Two mutexes |
| `Mutex` + `MutexGuard` | Lock a named mutex with a timeout, RAII | Yes | Mutex, created on the first lock |
| `SignalSemaphore` | Wake one task from another | Yes | Counting semaphore starting at 0, created on first use |
| `OsEventFlags` | Wait for any or all of up to 24 event bits | Yes | Event group, created in the constructor |
| `CriticalGuard` | Keep interrupts and the other core out of a few lines | No | None |

## RtosMutex

The mutex for new code. It is recursive (the owner may lock it again), satisfies the standard `Lockable`
requirements, so `std::lock_guard`, `std::unique_lock` and `std::scoped_lock` work, and can be moved. `Take()` and
`Give()` are aliases for code written against a semaphore-style API.

```cpp
--8<-- "tests/docs/sync.cpp:rtos-mutex"
```

| Method | Returns |
| --- | --- |
| `lock()` | True when locked (waits forever) |
| `try_lock()` | True when locked without waiting |
| `try_lock_for(ms)` | True when locked within `ms` |
| `unlock()` | Nothing |

`RtosSharedMutex` adds `lock_shared()` / `unlock_shared()` for readers; `RtosUniqueLock<M>` and
`RtosSharedLock<M>` are RAII locks with an optional timeout.

## Mutex and MutexGuard

A named mutex and its RAII guard. `MutexGuard` takes the mutex in its
constructor, waiting up to the given time (250 ms by default), reports through the optional `bool*` whether it
got it, and releases it in its destructor.

```cpp
--8<-- "tests/docs/sync.cpp:mutex-guard"
```

`Mutex::Lock(ms)` and `Mutex::Unlock()` are the direct calls. The kernel mutex is created on the first lock
(`EnsureInitialized()`).

## SignalSemaphore

A named semaphore that starts empty: one side calls `Signal()`, the other waits in `WaitUntilSignalled(ms)`. Each
`Signal()` releases one wait. `BaseThread` uses one to receive its start signal. The name is copied into a 40-byte
buffer in the object, and the kernel semaphore is created on first use.

```cpp
--8<-- "tests/docs/sync.cpp:semaphore"
```

## OsEventFlags

An event group with a typed wait mode. `Wait()` blocks until the requested bits arrive (`WaitMode::Any` or
`WaitMode::All`) or the timeout passes, and writes the bits it saw to its last argument.

!!! warning "Check the bits as well as the result"
    On the FreeRTOS backend `Wait()` currently returns true on a timeout too, because the backend's
    `os_event_group_get()` always reports success. Compare the returned bits with the ones you asked for, as the
    example does. With `WaitMode::All` FreeRTOS also clears the bits when the wait is satisfied; with
    `WaitMode::Any` they stay set until you `Clear()` them.

```cpp
--8<-- "tests/docs/sync.cpp:event-flags"
```

On FreeRTOS an event group holds 24 usable bits (the top 8 are reserved by the kernel). The template parameter
of `OsEventFlags<>` is unused and kept for source compatibility.

## CriticalGuard

Masks interrupts (and, on SMP FreeRTOS, takes the backend's spinlock so the other core waits) from construction to
destruction. Keep the guarded code to a few instructions: no blocking calls, no logging.

```cpp
--8<-- "tests/docs/sync.cpp:critical"
```

## Memory and timing

- After the kernel object exists (constructor or first use, as in the table above), no call allocates.
- Every blocking call takes a timeout in milliseconds; `UINT32_MAX` waits forever, 0 does not wait.
- None of these calls is for interrupt context.

Next: [Queues](queues.md).
