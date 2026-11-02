# Utility

Small helpers: wait for a condition with a timeout, read time and sleep, start a group of threads together, and
turn FreeRTOS codes into text.

## TestLogicWithTimeout

`TestLogicWithTimeout(func, expected, timeoutMsec, checkIntervalMs)` calls `func()` until it returns `expected`
or `timeoutMsec` passes, sleeping `checkIntervalMs` between calls. It returns true when the value arrived.
`BaseThread` uses it to verify starts and stops.

```cpp
--8<-- "tests/docs/utility.cpp:wait"
```

The check interval is passed to `os_delay_msec()` as a `uint16_t`. On the no-RTOS backend without an external
clock, time does not move, so the loop never times out: define `HF_RTOS_NONE_EXTERNAL_CLOCK` in host builds that
call it (see [Backends](backends.md#time-without-a-scheduler)).

## Time

| Call | Header | Returns |
| --- | --- | --- |
| `RtosTime::GetCurrentTimeUs()` | `RtosMutex.h` | Microseconds since start, from the kernel tick (tick resolution) or the external clock |
| `RtosTime::MsToTicks(ms)` | `RtosMutex.h` | Ticks, at least 1 for a non-zero `ms` |
| `os_get_elapsed_time_msec()` | `OsUtility.h` | Milliseconds since start, as `uint32_t` (wraps after 49.7 days) |
| `os_delay_msec(ms)` | `OsUtility.h` | Sleeps `ms` (a `uint16_t`, at most 65 535) |
| `os_delay_time(t, unit)` | `OsUtility.h` | Sleeps `t` in `TIME_UNIT_US`, `TIME_UNIT_MS` or `TIME_UNIT_S` |

```cpp
--8<-- "tests/docs/utility.cpp:time"
```

## BaseThreadsManager

Starts and stops a set of [`BaseThread`](base-thread.md) objects together, named by an enum, with or without
waiting for each to confirm. The enum must be unscoped (`enum`, not `enum class`) because its count also sizes a
`std::bitset`.

```cpp
--8<-- "tests/docs/utility.cpp:manager"
```

| Group | Calls |
| --- | --- |
| Start | `StartAll()`, `StartSelected(list)`, `StartAllExceptSelected(list)`, and each `...AndWaitToVerify(..., ms)` |
| Stop | `StopAll()`, `StopSelected(list)`, `StopAllExceptSelected(list)`, and each `...AndWaitToVerify(..., ms)` |
| Resume | `ResumeAll()`, `ResumeSelected(list)` |
| Hooks | `PreThreadInitializationActions()`, `PostThreadInitializationActions()` (virtual, override in a subclass) |

The registry is a `std::map` copied from the constructor argument, so it allocates on the heap once; an internal
`Mutex` guards it.

## FreeRTOSUtils

`freertos_ret_to_string(BaseType_t)` and `freertos_task_state_to_string(eTaskState)` return a name for a FreeRTOS
return code or task state, for logs. Without FreeRTOS they return `"N/A (no RTOS)"`. Compile
`src/FreeRTOSUtils.c` with the FreeRTOS backend.

Next: [API reference](api-reference.md).
