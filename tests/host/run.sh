#!/usr/bin/env sh
# Host tests for the RTOS backends. Run from the repository root.
#  1. The FreeRTOS backend and the C++ wrappers against the FreeRTOS shim in
#     tests/host/freertos-shim, with AddressSanitizer and UBSan.
#  2. Every source file and public header compiles for HF_RTOS_CMSIS, against
#     the declaration-only cmsis_os2.h in tests/host/cmsis.
#  3. No header name clashes with hf-utils-general: with a stand-in for its
#     Utility.h first on the include path (hf-core's order), every source file
#     and public header still compiles.
set -eu
CXX="${CXX:-c++}"
OUT="${OUT:-build/host-tests}"
mkdir -p "$OUT"

"$CXX" -std=c++17 -Wall -Wextra -Werror -g -O1 -fno-omit-frame-pointer \
  -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -DHF_RTOS_FREERTOS=1 -Iinclude -Itests/host/freertos-shim \
  tests/host/test_freertos_backend.cpp tests/host/freertos-shim/freertos_shim.cpp \
  src/OsUtility.cpp src/CriticalSection.cpp \
  -o "$OUT/test_freertos_backend"
"$OUT/test_freertos_backend"

for f in src/*.cpp; do
  "$CXX" -std=c++17 -fsyntax-only -DHF_RTOS_CMSIS=1 -Iinclude -Itests/host/cmsis "$f"
done
for h in include/*.h; do
  printf '#include "%s"\n' "$(basename "$h")" |
    "$CXX" -std=c++17 -fsyntax-only -x c++ -DHF_RTOS_CMSIS=1 -Iinclude -Itests/host/cmsis -
done
echo "ok   HF_RTOS_CMSIS: sources and headers compile"

for f in src/*.cpp; do
  "$CXX" -std=c++17 -fsyntax-only -DHF_RTOS_CMSIS=1 -Itests/host/name-clash -Iinclude -Itests/host/cmsis "$f"
done
for h in include/*.h; do
  printf '#include "%s"\n' "$(basename "$h")" |
    "$CXX" -std=c++17 -fsyntax-only -x c++ -DHF_RTOS_CMSIS=1 -Itests/host/name-clash -Iinclude \
      -Itests/host/cmsis -
done
echo "ok   no header name clashes with hf-utils-general (Utility.h)"
