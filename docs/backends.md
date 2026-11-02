# Backends

How a build picks the scheduler underneath, what each backend does differently, how to run with no RTOS, and how
to add a kernel.

## Selecting one

Define exactly one macro for the whole build (all sources that include these headers). `OsAbstraction.h` stops
the build with an error when more than one is defined.

| Definition | Backend header | Kernel headers | Core affinity | Critical section |
| --- | --- | --- | --- | --- |
| `HF_RTOS_FREERTOS=1` | [`os/os_freertos.h`](../include/os/os_freertos.h) | `freertos/FreeRTOS.h` (ESP-IDF) or flat `FreeRTOS.h`, probed with `__has_include` | Pinned to a core on SMP, plain create otherwise | Spinlock on SMP, interrupt masking otherwise |
| `HF_RTOS_THREADX=1` | [`os/os_threadx.h`](../include/os/os_threadx.h) | `tx_api.h` | None (single core) | `tx_interrupt_control` |
| `HF_RTOS_CMSIS=1` | [`os/os_cmsis.h`](../include/os/os_cmsis.h) | `cmsis_os2.h` | None (affinity lives in `osThreadAttr_t`) | `osKernelLock` |
| `HF_RTOS_NONE=1` | [`os/os_none.h`](../include/os/os_none.h) | None | None | No-op |

FreeRTOS is the backend in use today (ESP-IDF v5.5 on the ESP32-S3, FreeRTOS kernel 11.1).

!!! note "CMSIS-RTOS2 tick rate"
    CMSIS-RTOS2 reports its tick frequency only at run time, and the millisecond-to-tick conversion is
    `constexpr`, so the backend assumes 1 kHz. Define `HF_RTOS_CMSIS_TICK_RATE_HZ` to your kernel's tick rate if
    it differs. On FreeRTOS the conversion uses `configTICK_RATE_HZ`; on ThreadX it assumes 1 kHz.
    `BaseThread::ChangePriority()` and `GetStackHighWaterMark()` are not implemented for CMSIS-RTOS2 and return
    false and 0. CI compiles the CMSIS-RTOS2 build against a declaration-only `cmsis_os2.h`; it has not yet run on
    a CMSIS kernel.

## Event-flag waits

`os_event_group_get()` returns `OS_SUCCESS` only when the wait condition was met (`OS_AND`: all bits, `OS_OR`:
any of them) and an error on a timeout; the bits seen are written out either way. No backend clears bits unless
the option includes `OS_EVENT_CLEAR`, which clears the awaited bits when the wait succeeds.

## Capabilities that always exist

`os_thread_create_pinned` and `os_get_current_core_id` are present in every backend. On a single-core kernel the
first creates an unpinned thread and the second returns 0, so callers never need their own `#if`.

## FreeRTOS flavours

Two families ship under the same name:

- **Include layout.** ESP-IDF installs the headers under `freertos/`; CubeMX and upstream ship them flat. The
  backend probes for both.
- **SMP API.** `xTaskCreatePinnedToCore`, `tskNO_AFFINITY`, `portMUX_TYPE` and the two-argument
  `portENTER_CRITICAL` exist only on multi-core kernels. `HF_OS_FREERTOS_SMP` gates them; the backend derives it
  from `ESP_PLATFORM`, `portNUM_PROCESSORS` and `configNUM_CORES` after the kernel headers are in.

## Running without an RTOS

`HF_RTOS_NONE=1` is for host tests and bare-metal images. There is no scheduler, so the kernel calls become
stubs:

| Call | Result on the no-RTOS backend |
| --- | --- |
| Mutex lock and unlock, semaphore give and take, event-flag set, clear and wait | Succeed at once |
| `OsQueue::Send()` | Returns true; nothing is stored |
| `OsQueue::Receive()` | Returns false |
| Thread and timer create, start and stop | Return true; nothing runs |
| `CriticalGuard` | No-op |
| `BaseThread::StartThreadAndWaitToVerify()` | Runs `Setup()` inline and marks the thread running (`src/BaseThreadHostStub.cpp`); you call `Step()` |

Build `src/BaseThread.cpp`, `src/SignalSemaphore.cpp`, `src/RtosMutex.cpp` and `src/BaseThreadHostStub.cpp` for a
host build with threads.

### Time without a scheduler

By default there is no tick: `os_time_get()`, `os_get_elapsed_time_msec()` and `RtosTime::GetCurrentTimeUs()`
return 0 and every delay returns at once. Code that waits for a timeout by polling the clock then never times
out.

Define `HF_RTOS_NONE_EXTERNAL_CLOCK=1` to take time from the integrator, who then provides two C functions:

```c
uint64_t hf_os_none_clock_us(void);      /* monotonic microseconds */
void     hf_os_none_delay_us(uint64_t);  /* let that much time pass */
```

The hf-core host build backs them with a fake clock that advances on every delay, so timeouts expire without
real waiting. One tick is one millisecond, the same as `os_convert_msec_to_delay_ticks()` on this backend. A
bare-metal image can back them with a hardware timer.

## The porting contract

A backend owns every kernel difference. A consumer build must never force-include a compatibility header,
redefine a kernel macro or synthesise an include directory to make a backend compile. If a kernel variant does
not fit, extend its backend header. Build systems select a backend; they do not repair one.

## Adding a backend

<div class="hf-steps" markdown>

1. Add `include/os/os_<kernel>.h` implementing the whole `OS_*` / `os_*` surface. Copy the structure of
   `os_threadx.h`.
2. Add the selection branch and the table row to `include/OsAbstraction.h`.
3. Declare the `os_*_ex` helpers for it in `include/OsUtility.h` if the C++ classes are to build.
4. Add storage to `src/CriticalSection.cpp` if the backend declares `os_critical_mux`.
5. Add the row to [`include/os/README.md`](../include/os/README.md) and to the table on this page.

</div>

The [layer figure](architecture.md) reads the backend table from `OsAbstraction.h`, so step 2 updates it.

Next: [Integration](integration.md).
