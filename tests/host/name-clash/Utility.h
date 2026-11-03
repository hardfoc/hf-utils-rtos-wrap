/*
 * Stand-in for hf-utils-general's Utility.h (which also defines MIN and MAX).
 * hf-core puts hf-utils-general/include before hf-utils-rtos-wrap/include, so
 * an `#include "Utility.h"` in this repository that is not resolved next to the
 * including file reaches the other library's header. tests/host/run.sh puts
 * this directory first on the include path: nothing here may include it.
 */
#error "hf-utils-rtos-wrap included a header named Utility.h: the name clashes with hf-utils-general"
