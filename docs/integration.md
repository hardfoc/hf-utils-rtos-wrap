# Integration

Add the library to a firmware or host build: requirements, the sources per backend, the compile definitions and
how hf-core does it.

## Requirements

| Item | Requirement |
| --- | --- |
| Language | C++17 or later (the headers are also checked with C++20); `src/FreeRTOSUtils.c` is C |
| Kernel | FreeRTOS 11.1 (ESP-IDF v5.5), ThreadX, or none; see [Backends](backends.md) |
| Heap | Only where a kernel object is created, and `BaseThreadsManager`'s `std::map` |
| Dependencies | None outside the kernel; this library includes nothing from other HardFOC repositories |
| Tick rate | 1 kHz or faster for millisecond-precision delays and timeouts; see [Tick rate](#tick-rate) |

The repository has no build files of its own: the integrating project lists the sources.

## As part of hf-core

Most projects get this library through [hf-core](https://github.com/hardfoc/hf-core), which carries it in the
`hf-core-utils` submodule and compiles the right sources for its `HF_CORE_RTOS` setting (`FREERTOS`, `THREADX` or
`NONE`) in `cmake/hf_core_build_settings.cmake`. Nothing else is needed.

## Tick rate

HardFOC firmware runs FreeRTOS at **1 kHz or faster** (`CONFIG_FREERTOS_HZ=1000` in every HardFOC ESP-IDF
`sdkconfig`). Millisecond delays and timeouts are only millisecond-precise at that rate or above: one tick is then
1 ms or less, and `os_convert_msec_to_delay_ticks()` gives exactly `ms` ticks at 1 kHz.

On a slower tick the wrapper does not fail, it rounds. A non-zero millisecond value becomes at least one tick and
otherwise rounds down; 0 ms stays 0 ticks, a poll or a yield. At 100 Hz, `os_delay_msec(1)` and a 5 ms queue
timeout both wait one tick (10 ms), and 25 ms waits 2 ticks (20 ms). Code written for HardFOC can assume the
1 kHz rate; a project that runs slower gets these coarser waits.

The CMSIS-RTOS2 and ThreadX backends assume 1 kHz for the conversion; see [Backends](backends.md).

## In your own CMake project

```bash
git submodule add https://github.com/hardfoc/hf-utils-rtos-wrap.git external/hf-utils-rtos-wrap
```

=== "FreeRTOS (ESP-IDF)"

    ```cmake
    set(RTOS_WRAP ${CMAKE_CURRENT_LIST_DIR}/external/hf-utils-rtos-wrap)
    idf_component_register(
        SRCS
            ${RTOS_WRAP}/src/BaseThread.cpp
            ${RTOS_WRAP}/src/CriticalSection.cpp
            ${RTOS_WRAP}/src/FreeRTOSUtils.c
            ${RTOS_WRAP}/src/Mutex.cpp
            ${RTOS_WRAP}/src/MutexGuard.cpp
            ${RTOS_WRAP}/src/OsUtility.cpp
            ${RTOS_WRAP}/src/PeriodicTimer.cpp
            ${RTOS_WRAP}/src/RtosMutex.cpp
            ${RTOS_WRAP}/src/SignalSemaphore.cpp
        INCLUDE_DIRS ${RTOS_WRAP}/include
        REQUIRES freertos)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC HF_RTOS_FREERTOS=1)
    ```

=== "ThreadX"

    ```cmake
    set(RTOS_WRAP ${CMAKE_CURRENT_LIST_DIR}/external/hf-utils-rtos-wrap)
    add_library(hf_rtos_wrap STATIC
        ${RTOS_WRAP}/src/BaseThread.cpp
        ${RTOS_WRAP}/src/CriticalSection.cpp
        ${RTOS_WRAP}/src/Mutex.cpp
        ${RTOS_WRAP}/src/MutexGuard.cpp
        ${RTOS_WRAP}/src/OsUtility.cpp
        ${RTOS_WRAP}/src/PeriodicTimer.cpp
        ${RTOS_WRAP}/src/RtosMutex.cpp
        ${RTOS_WRAP}/src/SignalSemaphore.cpp)
    target_include_directories(hf_rtos_wrap PUBLIC ${RTOS_WRAP}/include)
    target_compile_definitions(hf_rtos_wrap PUBLIC HF_RTOS_THREADX=1)
    target_link_libraries(hf_rtos_wrap PUBLIC threadx)   # your ThreadX target
    ```

=== "CMSIS-RTOS2"

    ```cmake
    set(RTOS_WRAP ${CMAKE_CURRENT_LIST_DIR}/external/hf-utils-rtos-wrap)
    add_library(hf_rtos_wrap STATIC
        ${RTOS_WRAP}/src/BaseThread.cpp
        ${RTOS_WRAP}/src/CriticalSection.cpp
        ${RTOS_WRAP}/src/Mutex.cpp
        ${RTOS_WRAP}/src/MutexGuard.cpp
        ${RTOS_WRAP}/src/OsUtility.cpp
        ${RTOS_WRAP}/src/PeriodicTimer.cpp
        ${RTOS_WRAP}/src/RtosMutex.cpp
        ${RTOS_WRAP}/src/SignalSemaphore.cpp)
    target_include_directories(hf_rtos_wrap PUBLIC ${RTOS_WRAP}/include)
    target_compile_definitions(hf_rtos_wrap PUBLIC HF_RTOS_CMSIS=1)
    target_link_libraries(hf_rtos_wrap PUBLIC cmsis_rtos2)   # your CMSIS-RTOS2 kernel target
    ```

=== "Host (no RTOS)"

    ```cmake
    set(RTOS_WRAP ${CMAKE_CURRENT_LIST_DIR}/external/hf-utils-rtos-wrap)
    add_library(hf_rtos_wrap STATIC
        ${RTOS_WRAP}/src/RtosMutex.cpp
        ${RTOS_WRAP}/src/BaseThread.cpp
        ${RTOS_WRAP}/src/SignalSemaphore.cpp
        ${RTOS_WRAP}/src/BaseThreadHostStub.cpp)
    target_include_directories(hf_rtos_wrap PUBLIC ${RTOS_WRAP}/include)
    target_compile_definitions(hf_rtos_wrap PUBLIC
        HF_RTOS_NONE=1
        HF_RTOS_NONE_EXTERNAL_CLOCK=1)   # optional: your harness provides the clock
    ```

These are the source lists for each backend; hf-core uses the FreeRTOS, ThreadX and host ones. Make the definitions `PUBLIC`: every file that includes
the headers must see the same backend.

## Compile definitions

| Definition | Set by | Effect |
| --- | --- | --- |
| `HF_RTOS_FREERTOS` | You | FreeRTOS backend |
| `HF_RTOS_THREADX` | You | ThreadX backend |
| `HF_RTOS_CMSIS` | You | CMSIS-RTOS2 backend |
| `HF_RTOS_CMSIS_TICK_RATE_HZ` | You, with `HF_RTOS_CMSIS` | Kernel tick rate for millisecond conversions; default 1000 |
| `HF_RTOS_NONE` | You | No-RTOS backend: stubs for host tests and bare metal |
| `HF_RTOS_NONE_EXTERNAL_CLOCK` | You, with `HF_RTOS_NONE` | Time and delays come from `hf_os_none_clock_us()` / `hf_os_none_delay_us()` |
| `HF_MCU_FAMILY_NONE` | hf-core's host build | Same stubs as `HF_RTOS_NONE` in `OsUtility.h` |
| `HF_OS_FREERTOS_SMP` | The FreeRTOS backend | 1 on multi-core kernels; do not set it yourself |

Next: [API guide](api-guide.md).
