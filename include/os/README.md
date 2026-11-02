# RTOS backends

One backend header per scheduler. `OsAbstraction.h` selects exactly one and is
the only header consumers include.

| Backend | Selected by | Kernel headers | Affinity | Critical section |
|---------|-------------|----------------|----------|------------------|
| `os_cmsis.h` | `HF_RTOS_CMSIS` | `cmsis_os2.h` | none (kernel affinity is in `osThreadAttr_t`) | `osKernelLock` |
| `os_freertos.h` | `HF_RTOS_FREERTOS` | `freertos/FreeRTOS.h` (ESP-IDF) or flat `FreeRTOS.h` — probed with `__has_include` | pinned-to-core on SMP, plain create otherwise | spinlock on SMP, ISR masking otherwise |
| `os_threadx.h` | `HF_RTOS_THREADX` | `tx_api.h` | none (single core) | `tx_interrupt_control` |
| `os_none.h` | neither | none | none | no-op |

## Porting contract

A backend owns **every** kernel difference. The rule that matters:

> A consumer build must never force-include a compatibility header, redefine a
> kernel macro, or synthesize an include directory to make a backend compile.

If a kernel variant does not fit, extend its backend header. Build systems
select a backend; they do not repair one.

### Capabilities that always exist

`os_thread_create_pinned` and `os_get_current_core_id` are present in every
backend. On a single-core kernel the first degrades to an unpinned create and
the second returns 0. Callers therefore never need their own `#if`.

### FreeRTOS flavors

Two families ship under the same name and differ in ways that used to leak into
consumers:

- **Include layout** — ESP-IDF installs under `freertos/`; CubeMX and upstream
  ship flat. Probed, not assumed.
- **SMP API** — `xTaskCreatePinnedToCore`, `tskNO_AFFINITY`, `portMUX_TYPE` and
  the two-argument `portENTER_CRITICAL` exist only on multi-core kernels.
  `HF_OS_FREERTOS_SMP` gates them, derived from `ESP_PLATFORM`,
  `portNUM_PROCESSORS`, and `configNUM_CORES` after the kernel headers are in.

### Time without a scheduler

`os_none.h` has no tick. By default `os_time_get()`, `os_get_elapsed_time_msec()`
and `RtosTime::GetCurrentTimeUs()` return 0 and every delay returns at once.
Code that waits for a timeout by polling the clock therefore never times out.

Defining `HF_RTOS_NONE_EXTERNAL_CLOCK` hands time to the integrator, who then
provides two C functions:

```c
uint64_t hf_os_none_clock_us(void);      /* monotonic microseconds */
void     hf_os_none_delay_us(uint64_t);  /* let that much time pass */
```

The hf-core host build backs them with a fake clock that advances on delays,
so timeouts expire without real waiting. One tick is one millisecond, the
same as `os_convert_msec_to_delay_ticks()` for this backend. Without the
define nothing changes.

## Adding a backend

1. Add `os_<kernel>.h` implementing the full `OS_*` / `os_*` surface.
2. Add the selection branch to `OsAbstraction.h`.
3. Add storage to `src/CriticalSection.cpp` if the backend declares
   `os_critical_mux`.
4. Add the row to the table above.
