// Code shown on docs/generic-templates.md. Compiled by tools/docs/check-snippets.sh.
#include <cstddef>
#include <cstdint>

// --8<-- [start:flags]
#include "FlagsSaver.h"
#include "OsUtility.h"

enum class SystemFlag : uint8_t { kReady = 0, kFault, kCount };
constexpr std::size_t kSystemFlagCount = static_cast<std::size_t>(SystemFlag::kCount);

static hf::FlagsSaver<SystemFlag, kSystemFlagCount> system_flags;

// Producers get the write side, consumers the read side.
hf::FlagsWriter<SystemFlag, kSystemFlagCount>& flags_out = system_flags;
hf::FlagsReader<SystemFlag, kSystemFlagCount>& flags_in = system_flags;

void MarkReady() { flags_out.Set(SystemFlag::kReady, os_get_elapsed_time_msec()); }

bool WaitUntilReady() {
    flags_in.ClearWaitEvent();                         // forget changes already seen
    while (!flags_in.IsSet(SystemFlag::kReady)) {
        if (!flags_in.WaitForChange(/*timeout_ms=*/100)) {
            return false;                              // nothing changed in time
        }
        flags_in.ClearWaitEvent();                     // re-arm before checking again
    }
    return true;
}
// --8<-- [end:flags]

// --8<-- [start:snapshot]
#include "SeqlockSnapshot.h"

struct MotorState {                                    // trivially copyable
    float speed_rpm;
    float current_a;
    uint32_t timestamp_ms;
};

static hf::SeqlockSnapshot<MotorState> motor_state;

void PublishState(const MotorState& s) { motor_state.Publish(s); }  // one writer

MotorState LatestState() {                             // any number of readers
    MotorState out{};
    (void)motor_state.Read(out);                       // returns the sequence number read
    return out;
}
// --8<-- [end:snapshot]

// --8<-- [start:history]
#include "ErrorHistory.h"

struct ErrorRecord {
    uint32_t timestamp_ms;
    uint16_t code;
};

static hf::ErrorHistory<ErrorRecord, /*kCapacity=*/32> errors;

void Report(uint16_t code) {
    (void)errors.Push(ErrorRecord{os_get_elapsed_time_msec(), code});  // overwrites the oldest when full
}

std::size_t CopyErrors(ErrorRecord (&out)[32]) {
    return errors.Snapshot(out, 32);                   // oldest first
}
// --8<-- [end:history]
