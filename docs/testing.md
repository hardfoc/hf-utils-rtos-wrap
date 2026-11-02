# Testing and CI

What is checked, where, and how to run it yourself.

## In this repository

| Check | Where | Command |
| --- | --- | --- |
| Documentation code compiles against the headers (no-RTOS backend, warnings as errors) | Docs workflow, before the site build | `tools/docs/check-snippets.sh` |
| Site builds in strict mode, figure is current, Doxygen, offline link check, Lighthouse budget | Docs workflow (`.github/workflows/ci-docs-publish.yml`) | `mkdocs build --strict`, `hardfoc-docs check` |

The repository has no build files or unit tests of its own; the library is compiled and tested by the projects
that include it.

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
