#!/usr/bin/env sh
# Compile the code shown on the documentation pages (tests/docs/*.cpp) against the headers, with the no-RTOS
# backend, so every class, method and argument on the site exists. The headers are a system include: warnings
# fail the snippets, not the library. Run from the repository root.
set -eu
CXX="${CXX:-c++}"
for f in tests/docs/*.cpp; do
  "$CXX" -std=c++20 -Wall -Wextra -Werror -fsyntax-only -DHF_RTOS_NONE=1 -isystem include "$f"
  echo "ok  $f"
done
