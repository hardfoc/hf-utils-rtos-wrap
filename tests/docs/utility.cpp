// Code shown on docs/utility.md. Compiled by tools/docs/check-snippets.sh.
// --8<-- [start:wait]
#include <atomic>
#include "Utility.h"

static std::atomic<bool> link_up{false};

bool WaitForLink() {
    return TestLogicWithTimeout([] { return link_up.load(); }, true,
                                /*timeoutMsec=*/2000, /*checkIntervalMs=*/10);
}
// --8<-- [end:wait]

// --8<-- [start:time]
#include <cstdint>
#include "OsUtility.h"
#include "RtosMutex.h"

uint64_t MeasureUs() {
    const uint64_t t0 = RtosTime::GetCurrentTimeUs();
    os_delay_msec(5);
    return RtosTime::GetCurrentTimeUs() - t0;
}
// --8<-- [end:time]

// --8<-- [start:manager]
#include <map>
#include "BaseThread.h"
#include "BaseThreadsManager.h"

enum AppThread : int { kControl, kTelemetry, kAppThreadCount };  // unscoped: also the bitset size

static const char* AppThreadName(AppThread t) {
    return t == kControl ? "control" : "telemetry";
}

class AppThreads : public BaseThreadsManager<AppThread, kAppThreadCount> {
public:
    using BaseThreadsManager::BaseThreadsManager;
};

bool StartApp(BaseThread& control, BaseThread& telemetry) {
    static AppThreads threads({{kControl, &control}, {kTelemetry, &telemetry}}, AppThreadName);
    return threads.EnsureInitialized() && threads.StartAllAndWaitToVerify(/*waitToVerifyTimeoutMsec=*/1000);
}
// --8<-- [end:manager]
