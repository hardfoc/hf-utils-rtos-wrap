# Changelog

All notable changes to hf-utils-rtos-wrap. The library has no releases yet; changes are listed by the date they
reached `main`.

## Unreleased

- `os_convert_msec_to_delay_ticks()` returns at least one tick for a non-zero millisecond value (it still rounds
  down otherwise, and 0 ms is still 0 ticks), on every backend. So `os_delay_msec()` and every timeout that goes
  through it (`OsQueue`, `OsEventFlags`, `FlagsSaver`, `SeqlockSnapshot`, `Mutex`, `MutexGuard`, `PeriodicTimer`)
  never turns a 1 ms delay into a zero-tick wait on a slow tick: at 100 Hz, 1 to 19 ms is one tick.
  `os_convert_hz_to_delay_ticks()` likewise gives at least one tick for a frequency above the tick rate, and 0 for
  0 Hz instead of dividing by zero.
  Host tests check both at the shim's 100 Hz tick.
- Documentation moved to the HardFOC MkDocs site, with the code on the pages compiled in CI.
- FreeRTOS: `OsQueue` creates its queue with `xQueueCreateStatic()` in the object's inline storage, with `N` items
  of `sizeof(T)` rounded up to words. It used `xQueueCreate()` on the kernel heap with the storage size in bytes as
  the length. Items are copied through a word buffer, so a `T` whose size is not a multiple of 4 is no longer read
  or written past its end on any backend. On ThreadX the queue storage is now sized for `N` items.
- `os_event_group_get()` returns success only when the wait condition was met, so `OsEventFlags::Wait()` returns
  false on a timeout (FreeRTOS always reported success). No backend clears bits unless the new `OS_EVENT_CLEAR`
  option is given: FreeRTOS no longer clears them on an `OS_AND` wait, and CMSIS-RTOS2 no longer clears them on
  every wait.
- `FlagsSaver::WaitForChange()` and `SeqlockSnapshot::WaitForChange()` consume the change signal and convert the
  timeout from milliseconds to ticks.
- FreeRTOS: millisecond conversions use `configTICK_RATE_HZ` instead of assuming 1 kHz, in 64-bit arithmetic.
- The C++ classes build with `HF_RTOS_CMSIS` (`HF_RTOS_CMSIS_TICK_RATE_HZ` sets the tick rate, default 1000).
- Host tests: the FreeRTOS backend on a kernel shim, and a CMSIS-RTOS2 compile check (`tests/host/run.sh`).

## 2026-11-01

- The no-RTOS backend takes time and delays from the integrator with `HF_RTOS_NONE_EXTERNAL_CLOCK`
  (`hf_os_none_clock_us()`, `hf_os_none_delay_us()`), and `BaseThread.cpp` builds next to
  `BaseThreadHostStub.cpp` without an RTOS.

## 2026-10-31

- `RtosMutex` and `RtosSharedMutex` bodies moved into `src/RtosMutex.cpp`.

## 2026-08 and 2026-09

- CMSIS-RTOS2 backend (`os/os_cmsis.h`) and yield helpers; clean under `-Wpedantic -Werror`.
- ThreadX backend (`os/os_threadx.h`) and tick conversion for the no-RTOS backend.
- FreeRTOS included through `OsAbstraction.h` instead of a fixed include prefix.

## 2026-05

- Modernised API: `uint32_t` millisecond timeouts, `WaitMode`, eager `OsQueue` and `OsEventFlags`.
- Generic templates `hf::FlagsSaver`, `hf::SeqlockSnapshot`, `hf::ErrorHistory`.
- Host-only `BaseThread` stub for `HF_RTOS_NONE` builds.

## 2026-04

- `BaseThread` core-pinning overload, `os_thread_create_pinned` and `os_get_current_core_id`.
- `PeriodicTimer::Destroy()` safe when the timer was never created.
