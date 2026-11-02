# Changelog

All notable changes to hf-utils-rtos-wrap. The library has no releases yet; changes are listed by the date they
reached `main`.

## Unreleased

- Documentation moved to the HardFOC MkDocs site, with the code on the pages compiled in CI.

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
