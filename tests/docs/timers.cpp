// Code shown on docs/timers.md. Compiled by tools/docs/check-snippets.sh.
// --8<-- [start:timer]
#include <cstdint>
#include "PeriodicTimer.h"

static void Heartbeat(uint32_t arg) {
    (void)arg;                                         // runs in the timer service task
}

static PeriodicTimer heartbeat;

bool StartHeartbeat() {
    return heartbeat.Create("Heartbeat", Heartbeat, /*arg=*/0, /*periodMs=*/500,
                            /*autoStart=*/true);
}

void PauseHeartbeat() { heartbeat.Stop(); }
void ResumeHeartbeat() { heartbeat.Start(); }
// --8<-- [end:timer]
