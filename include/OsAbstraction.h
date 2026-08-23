#pragma once
/**
 * @file OsAbstraction.h
 * @brief Generic RTOS abstraction — public header.
 *
 * Exposes one set of `OS_*` types and `os_*` functions regardless of the
 * scheduler underneath, so component code never names a kernel. Select a
 * backend by defining exactly one of:
 *
 * | Macro | Backend | Header |
 * |-------|---------|--------|
 * | `HF_RTOS_CMSIS`    | CMSIS-RTOS2 (`osThreadNew` / `osDelay`) | `os/os_cmsis.h` |
 * | `HF_RTOS_FREERTOS` | FreeRTOS (vanilla or ESP-IDF SMP) | `os/os_freertos.h` |
 * | `HF_RTOS_THREADX`  | Azure RTOS ThreadX | `os/os_threadx.h` |
 * | *(none)*           | Bare-metal / host tests | `os/os_none.h` |
 *
 * ## Porting contract
 *
 * A backend owns every kernel difference — include layout, core affinity,
 * critical-section style, tick source. Consumers must never need a
 * force-included compatibility header or a synthesized include directory to
 * make a backend compile. If a kernel variant does not fit, extend its
 * backend header; do not patch it from a build system.
 *
 * Capability macros a backend must define:
 * - `os_thread_create_pinned` always exists; it degrades to an unpinned
 *   create when the kernel is single-core.
 * - `os_get_current_core_id` always exists; it returns 0 when single-core.
 *
 * @see os/README.md for the backend matrix and how to add one.
 */

/* ── RTOS selection guard ────────────────────────────────────────────────── */
#if (defined(HF_RTOS_FREERTOS) + defined(HF_RTOS_THREADX) + defined(HF_RTOS_CMSIS)) > 1
#  error "Select exactly one RTOS backend: HF_RTOS_CMSIS, HF_RTOS_FREERTOS, or HF_RTOS_THREADX"
#endif

#if defined(HF_RTOS_CMSIS)
#  include "os/os_cmsis.h"
#elif defined(HF_RTOS_FREERTOS)
#  include "os/os_freertos.h"
#elif defined(HF_RTOS_THREADX)
#  include "os/os_threadx.h"
#else /* HF_RTOS_NONE (or no RTOS selected) — bare-metal / host-only stubs */
#  include "os/os_none.h"
#endif
