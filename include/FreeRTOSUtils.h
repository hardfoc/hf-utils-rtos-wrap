/**
 * @file FreeRTOSUtils.h
 * @brief FreeRTOS utility functions for error reporting and debugging.
 *
 * @todo Add @copyright line once project copyright wording is finalised.
 */

#ifndef FREERTOS_UTILS_H_
#define FREERTOS_UTILS_H_

#if defined(HF_RTOS_FREERTOS)

/* The FreeRTOS backend owns the kernel include layout (flat vs `freertos/`
 * prefixed) — see os/README.md. Pull it in rather than repeating the probe. */
#include "OsAbstraction.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert FreeRTOS return code to a human-readable string.
 * @param result The FreeRTOS return code
 * @return String representation of the error code
 */
const char* freertos_ret_to_string(BaseType_t result);

/**
 * @brief Convert FreeRTOS task state to a human-readable string.
 * @param state The task state
 * @return String representation of the task state
 */
const char* freertos_task_state_to_string(eTaskState state);

#ifdef __cplusplus
}
#endif

#else /* HF_RTOS_NONE — no FreeRTOS utilities available */

#ifdef __cplusplus
extern "C" {
#endif

static inline const char* freertos_ret_to_string(int result) { (void)result; return "N/A (no RTOS)"; }
static inline const char* freertos_task_state_to_string(int state) { (void)state; return "N/A (no RTOS)"; }

#ifdef __cplusplus
}
#endif

#endif /* HF_RTOS_FREERTOS */

#endif /* FREERTOS_UTILS_H_ */
