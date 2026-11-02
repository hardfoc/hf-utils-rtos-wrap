# Quickstart

Add the library to a build, write a worker thread and start it: the smallest complete use.

<div class="hf-steps" markdown>

1. **Add the sources.** Put `include/` on the include path and compile the files in `src/` (see
   [Integration](integration.md) for the list per backend).
2. **Select the backend.** Define `HF_RTOS_FREERTOS=1` for ESP-IDF, or `HF_RTOS_NONE=1` for a host build
   (see [Backends](backends.md)).
3. **Derive from `BaseThread`** and implement five methods.
4. **Initialise and start it**, and check both results.

</div>

## A worker thread

```cpp title="tests/docs/base_thread.cpp"
--8<-- "tests/docs/base_thread.cpp:thread"
```

```cpp
--8<-- "tests/docs/base_thread.cpp:start"
```

## What each step does

| Call | What happens |
| --- | --- |
| `EnsureInitialized()` | Calls your `Initialize()` once, which creates the task with `CreateBaseThread()` on the stack you own |
| `StartThreadAndWaitToVerify(1000)` | Signals the task to start and waits up to 1000 ms until it reports running |
| (task) | Runs `ResetVariables()`, then `Setup()`, then `Step()` repeatedly, sleeping the returned milliseconds in between |
| `StopThreadAndWaitToVerify(1000)` | Requests a stop; the task finishes the current `Step()`, runs `Cleanup()` and waits to be started again |

## If it does not work

| Symptom | Check |
| --- | --- |
| `EnsureInitialized()` returns false | The stack is too small for the kernel's minimum, or the priority is out of range |
| `StartThreadAndWaitToVerify()` returns false | The task never ran: a higher-priority task hogs the core, or the scheduler is not started |
| `Step()` never runs on a host build | Expected: without a scheduler there is no task. Call `Step()` yourself (see [BaseThread](base-thread.md#on-a-host-build)) |
| Link errors for `hf_os_none_clock_us` | `HF_RTOS_NONE_EXTERNAL_CLOCK` is defined but your harness does not provide the two clock functions |

Next: [Architecture](architecture.md).
