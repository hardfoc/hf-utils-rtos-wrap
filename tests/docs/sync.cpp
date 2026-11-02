// Code shown on docs/synchronization.md. Compiled by tools/docs/check-snippets.sh.
#include <cstdint>
#include <mutex>

// --8<-- [start:rtos-mutex]
#include "RtosMutex.h"

static RtosMutex config_mutex;
static int config_value = 0;

void SetConfig(int v) {
    std::lock_guard<RtosMutex> lock(config_mutex);     // unlocks at the end of the scope
    config_value = v;
}

bool TrySetConfig(int v) {
    if (!config_mutex.try_lock_for(/*timeout_ms=*/10)) {
        return false;
    }
    config_value = v;
    config_mutex.unlock();
    return true;
}
// --8<-- [end:rtos-mutex]

// --8<-- [start:mutex-guard]
#include "Mutex.h"
#include "MutexGuard.h"

static Mutex bus_mutex("Bus");

bool WriteBus() {
    bool locked = false;
    MutexGuard guard(bus_mutex, /*maxMsecToWait=*/50, &locked);
    if (!locked) {
        return false;                                  // another task held it for 50 ms
    }
    // talk to the bus
    return true;
}
// --8<-- [end:mutex-guard]

// --8<-- [start:semaphore]
#include "SignalSemaphore.h"

static SignalSemaphore data_ready("DataReady");

void OnDataReady() { data_ready.Signal(); }              // producer task

bool WaitForData() {
    return data_ready.WaitUntilSignalled(/*msecToWait=*/200);  // consumer task
}
// --8<-- [end:semaphore]

// --8<-- [start:event-flags]
#include "OsEventFlags.h"

constexpr uint32_t kAdcDone = 1u << 0;
constexpr uint32_t kEncoderDone = 1u << 1;

static OsEventFlags<> sensors("Sensors");

void AdcFinished() { sensors.Set(kAdcDone); }
void EncoderFinished() { sensors.Set(kEncoderDone); }

bool WaitForBoth() {
    uint32_t bits = 0;
    if (!sensors.Wait(kAdcDone | kEncoderDone, WaitMode::All, /*timeout_ms=*/5, bits)) {
        return false;                                  // bits holds what did arrive
    }
    sensors.Clear(kAdcDone | kEncoderDone);
    return true;
}
// --8<-- [end:event-flags]

// --8<-- [start:critical]
#include "CriticalGuard.h"

static volatile uint32_t isr_counter = 0;

uint32_t ReadAndResetCounter() {
    CriticalGuard guard;                               // interrupts masked until the end of scope
    const uint32_t n = isr_counter;
    isr_counter = 0;
    return n;
}
// --8<-- [end:critical]
