[🏠 Back to README](../README.md) | [➡️ Next](BaseThread.md)

# hf-utils-rtos-wrap Documentation

Guides for the RTOS wrapper library. Each page links to the next so you can
read straight through, or jump in via the table of contents below.

## Table of Contents
1. [Threading with `BaseThread`](BaseThread.md)
2. [Synchronization Primitives](Synchronization.md) — `RtosMutex`, `Mutex`, `MutexGuard`, `SignalSemaphore`, `OsEventFlags`, `CriticalGuard`
3. [Message Queues](Queues.md) — `OsQueue<T, Capacity>`
4. [Generic Templates](GenericTemplates.md) — `hf::FlagsSaver`, `hf::SeqlockSnapshot`, `hf::ErrorHistory`
5. [RTOS Abstraction](RTOSAbstraction.md) — the C portability layer (`OsAbstraction.h` / `OsUtility.h`)
6. [Periodic Timers](Timers.md) — `PeriodicTimer`
7. [Utility Helpers](Utility.md) — `WaitForCondition`, time helpers, `BaseThreadsManager`

For a one-line summary of every header in the library see the
[README header table](../README.md#header-summary).

[🏠 Back to README](../README.md) | [➡️ Next](BaseThread.md)
