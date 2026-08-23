#include "OsAbstraction.h"

/* Storage for the backend's critical-section state. FreeRTOS SMP needs a real
 * spinlock; single-core FreeRTOS keeps a placeholder so this file does not have
 * to know which flavor was selected. ThreadX stashes the saved interrupt
 * posture here. The no-scheduler backend needs no storage at all. */
#if defined(HF_RTOS_FREERTOS)
#  if HF_OS_FREERTOS_SMP
OS_Critical os_critical_mux = portMUX_INITIALIZER_UNLOCKED;
#  else
OS_Critical os_critical_mux = 0;
#  endif
#elif defined(HF_RTOS_THREADX) || defined(HF_RTOS_CMSIS)
OS_Critical os_critical_mux = 0;
#endif
