# Testing and CI

What is checked, where, and how to run it yourself.

## In this repository

| Check | Where | Command |
| --- | --- | --- |
| Documentation code compiles against the headers (no-RTOS backend, warnings as errors) | Docs workflow, before the site build | `tools/docs/check-snippets.sh` |
| Site builds in strict mode, figure is current, Doxygen, offline link check, Lighthouse budget | Docs workflow (`.github/workflows/ci-docs-publish.yml`) | `mkdocs build --strict`, `hardfoc-docs check` |
| FreeRTOS backend and the C++ wrappers on a host FreeRTOS shim (AddressSanitizer, UBSan); every source and header compiles for `HF_RTOS_CMSIS` | Host tests workflow (`.github/workflows/ci-host-tests.yml`) | `tests/host/run.sh` |

The repository has no build files of its own; the projects that include it compile it for their targets.

## Host tests

[`tests/host/run.sh`](../tests/host/run.sh) builds
[`tests/host/test_freertos_backend.cpp`](../tests/host/test_freertos_backend.cpp) with `HF_RTOS_FREERTOS`
against [`tests/host/freertos-shim/`](../tests/host/freertos-shim/FreeRTOS.h): stand-in kernel headers with a
100 Hz tick and static allocation, and a single-threaded model of queues and event groups that records the
arguments it was given. A blocking call never blocks; it returns as if its timeout had passed. The tests cover
`OsQueue` storage, capacity and item copies, `OsEventFlags::Wait()` results, and `WaitForChange()` on
`FlagsSaver` and `SeqlockSnapshot`, including the millisecond-to-tick conversion.

The script then compiles every source file and public header with `HF_RTOS_CMSIS` against a declaration-only
[`cmsis_os2.h`](../tests/host/cmsis/cmsis_os2.h), so the CMSIS-RTOS2 build cannot break unnoticed.

```bash
tests/host/run.sh                       # CXX=g++ tests/host/run.sh for GCC
```

## In hf-core

[hf-core](https://github.com/hardfoc/hf-core) compiles these sources for every backend it supports and runs host
tests against them with `HF_RTOS_NONE` and a fake clock:

| Test | Covers |
| --- | --- |
| `tests/host/test_rtos_host.cpp` | Delays advance the fake clock, `RtosTime` follows it, polling loops terminate, `RtosMutex`, `BaseThread` start, `Step()` and stop on the host |
| `examples/esp32/main/utils_tests/rtos_wrap_comprehensive_test.cpp` | Every class on an ESP32 with FreeRTOS: mutexes, guards, timers, threads, queues, event flags, semaphores |

Run the host tests from an hf-core checkout as its [host build guide](https://github.com/hardfoc/hf-core/blob/main/docs/host-build.md)
describes.

## Documentation code

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/base_thread.cpp). The docs
workflow compiles each file before building the site, so a change to a header that breaks a page fails the pull
request.

```bash
tools/docs/check-snippets.sh            # CXX=g++ tools/docs/check-snippets.sh for GCC
```
