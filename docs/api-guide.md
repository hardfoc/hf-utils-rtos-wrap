# API guide

Which class to reach for, by task. Each row links the page with a compiled example and the memory and timing
notes.

| Task | Use | Header | Page |
| --- | --- | --- | --- |
| Run a loop in its own task, start and stop it | `BaseThread` | `BaseThread.h` | [BaseThread](base-thread.md) |
| Start and stop a group of threads | `BaseThreadsManager<Enum, Max>` | `BaseThreadsManager.h` | [Utility](utility.md#basethreadsmanager) |
| Protect shared data | `RtosMutex` with `std::lock_guard` | `RtosMutex.h` | [Synchronization](synchronization.md#rtosmutex) |
| Lock with a timeout and a named mutex | `Mutex` and `MutexGuard` | `Mutex.h`, `MutexGuard.h` | [Synchronization](synchronization.md#mutex-and-mutexguard) |
| Wake one task from another | `SignalSemaphore` | `SignalSemaphore.h` | [Synchronization](synchronization.md#signalsemaphore) |
| Wait for any or all of several events | `OsEventFlags` | `OsEventFlags.h` | [Synchronization](synchronization.md#oseventflags) |
| Keep an interrupt out of a few lines | `CriticalGuard` | `CriticalGuard.h` | [Synchronization](synchronization.md#criticalguard) |
| Pass values between tasks | `OsQueue<T, N>` | `OsQueue.h` | [Queues](queues.md) |
| Call a function every N ms | `PeriodicTimer` | `PeriodicTimer.h` | [Timers](timers.md) |
| Share status flags | `hf::FlagsSaver<Id, N>` | `FlagsSaver.h` | [Generic templates](generic-templates.md#flagssaver) |
| Share the latest value of a struct | `hf::SeqlockSnapshot<T>` | `SeqlockSnapshot.h` | [Generic templates](generic-templates.md#seqlocksnapshot) |
| Keep the last N errors | `hf::ErrorHistory<R, N>` | `ErrorHistory.h` | [Generic templates](generic-templates.md#errorhistory) |
| Wait until a condition holds, with a timeout | `TestLogicWithTimeout()` | `RtosUtility.h` | [Utility](utility.md#testlogicwithtimeout) |
| Read time, sleep | `RtosTime::GetCurrentTimeUs()`, `os_delay_msec()` | `RtosMutex.h`, `OsUtility.h` | [Utility](utility.md#time) |

## Conventions

- **Results are `bool`.** True means the operation happened; false means a timeout or a failed kernel call.
- **Timeouts are milliseconds** in a `uint32_t`. `UINT32_MAX` waits forever; 0 does not wait.
- **Objects are not copyable.** Each owns a kernel handle; `RtosMutex` and `RtosSharedMutex` can be moved.
- **Task context only.** The queue, event-flag, mutex, semaphore and generic-template calls use the task-level
  kernel API. From an interrupt, use the kernel's `FromISR` calls and hand the work to a task.

Next: [BaseThread](base-thread.md).
