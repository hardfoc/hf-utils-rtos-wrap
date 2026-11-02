# Timers

`PeriodicTimer`: call a function at a fixed interval from the kernel's timer service, without a task of your own.

## Purpose

Heartbeats, watchdog kicks, polling a status pin: short work on a fixed period. The callback runs in the kernel
timer task, so it must be short and must not block.

## Example

```cpp
--8<-- "tests/docs/timers.cpp:timer"
```

## API

| Member | Does | Returns |
| --- | --- | --- |
| `Create(name, callback, arg, periodMs, autoStart = false)` | Creates the kernel timer; starts it when `autoStart` | False if already created or the kernel refused |
| `Start()` / `Stop()` | Activate or deactivate | True on success |
| `Destroy()` | Stops and deletes the kernel timer; safe when never created | True on success |
| `IsValid()` | | True between a successful `Create()` and `Destroy()` |

The callback signature is `void (*)(uint32_t)`; `arg` is passed to it on every call. The destructor deletes the
timer.

## Memory and timing

- **One kernel timer per object**, created in `Create()`. The object itself holds a handle and a flag.
- **The period is converted to ticks** with `os_convert_msec_to_delay_ticks()`, `ms * tick_rate / 1000`, rounded
  down: on a 100 Hz tick 25 ms becomes 2 ticks (20 ms) and anything under 10 ms becomes 0. Use whole ticks.
  The tick rate is `configTICK_RATE_HZ` on FreeRTOS.
- **Callbacks share one task.** A slow callback delays every other timer in the system; hand longer work to a
  [BaseThread](base-thread.md) or a [queue](queues.md).

Next: [Generic templates](generic-templates.md).
