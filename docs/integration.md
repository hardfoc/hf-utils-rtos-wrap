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

The repository has no build files of its own: the integrating project lists the sources.

## As part of hf-core

Most projects get this library through [hf-core](https://github.com/hardfoc/hf-core), which carries it in the
`hf-core-utils` submodule and compiles the right sources for its `HF_CORE_RTOS` setting (`FREERTOS`, `THREADX` or
`NONE`) in `cmake/hf_core_build_settings.cmake`. Nothing else is needed.

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

These are the source lists hf-core uses for each backend. Make the definitions `PUBLIC`: every file that includes
the headers must see the same backend.

## Compile definitions

| Definition | Set by | Effect |
| --- | --- | --- |
| `HF_RTOS_FREERTOS` | You | FreeRTOS backend |
| `HF_RTOS_THREADX` | You | ThreadX backend |
| `HF_RTOS_CMSIS` | You | CMSIS-RTOS2 backend (C layer only) |
| `HF_RTOS_NONE` | You | No-RTOS backend: stubs for host tests and bare metal |
| `HF_RTOS_NONE_EXTERNAL_CLOCK` | You, with `HF_RTOS_NONE` | Time and delays come from `hf_os_none_clock_us()` / `hf_os_none_delay_us()` |
| `HF_MCU_FAMILY_NONE` | hf-core's host build | Same stubs as `HF_RTOS_NONE` in `OsUtility.h` |
| `HF_OS_FREERTOS_SMP` | The FreeRTOS backend | 1 on multi-core kernels; do not set it yourself |

Next: [API guide](api-guide.md).
