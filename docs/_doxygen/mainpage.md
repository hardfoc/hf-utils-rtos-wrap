# RTOS Wrapper API reference {#mainpage}

The classes, functions and macros of the HardFOC RTOS wrapper, generated from the headers.

Start with BaseThread, RtosMutex, OsQueue and OsEventFlags; the shared-state templates are in the hf namespace
(hf::FlagsSaver, hf::SeqlockSnapshot, hf::ErrorHistory). The C portability layer is OsAbstraction.h and
OsUtility.h; the backends are in include/os/.

The guides, with examples that compile, are on the documentation site: <a href="../">RTOS Wrapper
documentation</a>.
