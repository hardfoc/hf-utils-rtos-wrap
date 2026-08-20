#include "OsAbstraction.h"

#if defined(HF_RTOS_FREERTOS)
OS_Critical os_critical_mux = portMUX_INITIALIZER_UNLOCKED;
#elif defined(HF_RTOS_THREADX)
OS_Critical os_critical_mux = 0;
#endif
