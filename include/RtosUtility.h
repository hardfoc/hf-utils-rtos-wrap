#pragma once
/**
 * @file RtosUtility.h
 * @brief Miscellaneous utility helpers for the RTOS wrapper.
 *
 * Named RtosUtility.h (it was Utility.h) so it cannot be confused with
 * hf-utils-general's Utility.h, which comes first on hf-core's include path
 * and also defines the MIN / MAX macros.
 */

#include <stdint.h>
#include <functional>
#include "OsUtility.h"

// `enum time_unit_t` is declared in OsUtility.h (must be visible to its
// C-facing function declarations); included transitively above.

/**
 * @brief Helper to repeatedly check a condition until timeout.
 *
 * @tparam Func  Callable returning a comparable value.
 * @tparam T     Type of the expected value.
 * @param func            Function returning a value to compare.
 * @param expected        Expected value from the function.
 * @param timeoutMsec     Maximum time in milliseconds to wait.
 * @param checkIntervalMs Delay between checks in milliseconds.
 * @return true if the expected value was returned before timeout.
 */
template <typename Func, typename T>
static inline bool TestLogicWithTimeout(Func func, T expected, uint32_t timeoutMsec, uint32_t checkIntervalMs)
{
    uint32_t start = os_get_elapsed_time_msec();
    while ((os_get_elapsed_time_msec() - start) < timeoutMsec) {
        if (func() == expected) {
            return true;
        }
        os_delay_msec(static_cast<uint16_t>(checkIntervalMs));
    }
    return false;
}

