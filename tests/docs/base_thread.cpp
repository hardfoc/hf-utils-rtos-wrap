// Code shown on docs/base-thread.md and docs/quickstart.md. Compiled by tools/docs/check-snippets.sh.
// --8<-- [start:thread]
#include "BaseThread.h"

class BlinkThread : public BaseThread {
public:
    BlinkThread() : BaseThread("Blink") {}

protected:
    bool Initialize() noexcept override {             // creates the task once
        return CreateBaseThread(stack_, sizeof(stack_), /*priority=*/5,
                                /*preempt_threshold=*/5, /*timeSliceAllowed=*/0,
                                OS_AUTO_START);
    }
    bool Setup() noexcept override { on_ = false; return true; }  // every start
    uint32_t Step() noexcept override {               // the loop body
        on_ = !on_;                                    // toggle an LED here
        return 500;                                    // ms until the next Step()
    }
    bool Cleanup() noexcept override { ClearSetupComplete(); return true; }
    bool ResetVariables() noexcept override { on_ = false; return true; }

private:
    bool on_ = false;
    uint8_t stack_[4096];                              // no heap: the stack lives here
};
// --8<-- [end:thread]

// --8<-- [start:start]
static BlinkThread blink;

bool StartBlink() {
    if (!blink.EnsureInitialized()) {                  // Initialize(): create the task
        return false;
    }
    return blink.StartThreadAndWaitToVerify(/*startTimeoutMsec=*/1000);
}

bool StopBlink() {
    return blink.StopThreadAndWaitToVerify(/*stopTimeoutMsec=*/1000);
}
// --8<-- [end:start]
