// Code shown on docs/queues.md. Compiled by tools/docs/check-snippets.sh.
// --8<-- [start:queue]
#include <cstdint>
#include "OsQueue.h"

struct Sample {
    uint32_t timestamp_ms;
    float current_a;
};

static OsQueue<Sample, /*kCapacity=*/32> samples("Samples");

bool Produce(const Sample& s) {
    return samples.Send(s, /*timeout_ms=*/0);          // do not block: drop when full
}

void Consume() {
    Sample s{};
    while (samples.Receive(s, /*timeout_ms=*/100)) {   // false after 100 ms with nothing
        // use s
    }
}
// --8<-- [end:queue]

bool QueueReady() { return samples.IsValid(); }
