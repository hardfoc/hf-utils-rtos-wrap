/*
 * The FreeRTOS backend and the C++ wrappers on top of it, against the host
 * FreeRTOS shim in freertos-shim/ (100 Hz tick, static allocation on).
 * Built and run by tests/host/run.sh.
 */
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "FlagsSaver.h"
#include "OsUtility.h"
#include "OsEventFlags.h"
#include "OsQueue.h"
#include "SeqlockSnapshot.h"
#include "freertos_shim.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

struct Tiny {  // 3 bytes, alignment 1: not a multiple of the 4-byte kernel word
    uint8_t a, b, c;
};

struct Guarded {  // canaries on both sides of the item a receive writes into
    uint8_t before[4];
    Tiny item;
    uint8_t after[5];
};

bool InObject(const void* p, const void* obj, size_t size) {
    const auto* b = static_cast<const uint8_t*>(obj);
    const auto* q = static_cast<const uint8_t*>(p);
    return q >= b && q < b + size;
}

void QueueUsesInlineStorageWithItemCountLength() {
    shim::Reset();
    OsQueue<Tiny, 5> q("tiny");
    CHECK(q.IsValid());
    const auto& c = shim::State().last_queue;
    CHECK(c.is_static);
    CHECK(shim::State().heap_queue_creates == 0);
    CHECK(c.length == 5);
    CHECK(c.item_size == 4);
    CHECK(InObject(c.storage, &q, sizeof(q)));
    CHECK(InObject(c.control, &q, sizeof(q)));
    CHECK(reinterpret_cast<uintptr_t>(c.control) % alignof(StaticQueue_t) == 0);
    // The items must fit inside the object after the control block.
    CHECK(c.storage != nullptr &&
          InObject(static_cast<const uint8_t*>(c.storage) + 5 * 4 - 1, &q, sizeof(q)));
}

void QueueHoldsExactlyItsCapacity() {
    shim::Reset();
    OsQueue<Tiny, 3> q("cap");
    for (uint8_t i = 0; i < 3; ++i) {
        CHECK(q.Send(Tiny{i, i, i}, 0));
    }
    CHECK(!q.Send(Tiny{9, 9, 9}, 0));  // full after N items
}

void QueueCopiesOnlyTheItem() {
    shim::Reset();
    OsQueue<Tiny, 4> q("copy");
    CHECK(q.Send(Tiny{1, 2, 3}, 0));
    CHECK(q.Send(Tiny{4, 5, 6}, 0));

    Guarded g;
    std::memset(&g, 0xA5, sizeof(g));
    CHECK(q.Receive(g.item, 0));
    CHECK(g.item.a == 1 && g.item.b == 2 && g.item.c == 3);
    for (uint8_t v : g.before) CHECK(v == 0xA5);
    for (uint8_t v : g.after) CHECK(v == 0xA5);

    CHECK(q.Receive(g.item, 0));
    CHECK(g.item.a == 4 && g.item.b == 5 && g.item.c == 6);
    for (uint8_t v : g.after) CHECK(v == 0xA5);

    CHECK(!q.Receive(g.item, 0));  // empty
}

void QueueTimeoutIsConvertedToTicks() {
    shim::Reset();
    OsQueue<uint32_t, 2> q("ticks");
    uint32_t v = 0;
    CHECK(!q.Receive(v, 250));
    CHECK(shim::State().last_queue_wait == 25);  // 250 ms at 100 Hz
    CHECK(!q.Receive(v, UINT32_MAX));
    CHECK(shim::State().last_queue_wait == portMAX_DELAY);
}

void EventWaitTimesOutWithFalse() {
    shim::Reset();
    OsEventFlags<> ev("ev");
    uint32_t seen = 123;
    CHECK(!ev.Wait(0x1, WaitMode::Any, 50, seen));
    CHECK(seen == 0);
    CHECK(shim::State().last_event_wait == 5);  // 50 ms at 100 Hz

    CHECK(ev.Set(0x2));
    CHECK(!ev.Wait(0x3, WaitMode::All, 0, seen));  // only one of the two bits
    CHECK(seen == 0x2);
    CHECK(ev.Wait(0x3, WaitMode::Any, 0, seen));
    CHECK(seen == 0x2);
}

void EventWaitDoesNotClearBits() {
    shim::Reset();
    OsEventFlags<> ev("keep");
    uint32_t seen = 0;
    CHECK(ev.Set(0x5));
    CHECK(ev.Wait(0x5, WaitMode::All, 0, seen));
    CHECK(shim::State().last_event_clear == pdFALSE);
    CHECK(ev.Wait(0x5, WaitMode::All, 0, seen));  // still set
    CHECK(ev.Wait(0x1, WaitMode::Any, 0, seen));
    CHECK(ev.Clear(0x5));
    CHECK(!ev.Wait(0x1, WaitMode::Any, 0, seen));
}

enum class Flag : uint8_t { A, B };

void FlagsSaverWaitConsumesTheChange() {
    shim::Reset();
    hf::FlagsSaver<Flag, 2> flags;
    CHECK(!flags.WaitForChange(30));  // nothing changed yet
    CHECK(shim::State().last_event_wait == 3);  // 30 ms at 100 Hz, not 30 ticks
    CHECK(flags.Set(Flag::A));
    CHECK(flags.WaitForChange(30));
    CHECK(!flags.WaitForChange(30));  // the change was consumed
    CHECK(flags.Clear(Flag::A));
    CHECK(flags.WaitForChange(0));
}

void SeqlockWaitConsumesTheChange() {
    shim::Reset();
    hf::SeqlockSnapshot<uint32_t> snap;
    snap.Publish(7);
    CHECK(snap.WaitForChange(1000));
    CHECK(shim::State().last_event_wait == 100);
    CHECK(!snap.WaitForChange(1000));
    snap.Publish(8);
    CHECK(snap.WaitForChange(UINT32_MAX));
    CHECK(shim::State().last_event_wait == portMAX_DELAY);
}

// The shim ticks at 100 Hz, so one tick is 10 ms.
static_assert(os_convert_msec_to_delay_ticks(0) == 0, "0 ms is no wait");
static_assert(os_convert_msec_to_delay_ticks(1) == 1, "a non-zero ms is at least one tick");
static_assert(os_convert_msec_to_delay_ticks(9) == 1, "a non-zero ms is at least one tick");
static_assert(os_convert_msec_to_delay_ticks(10) == 1, "exact multiple");
static_assert(os_convert_msec_to_delay_ticks(25) == 2, "longer delays still round down");
static_assert(os_convert_msec_to_delay_ticks(UINT32_MAX) == 429496729U, "64-bit product");
static_assert(os_convert_hz_to_delay_ticks(0) == 0, "0 Hz has no period");
static_assert(os_convert_hz_to_delay_ticks(1000) == 1, "faster than the tick is one tick");
static_assert(os_convert_hz_to_delay_ticks(50) == 2, "50 Hz at 100 Hz");

void ShortDelaysSleepAtLeastOneTick() {
    shim::Reset();
    os_delay_msec(1);
    CHECK(shim::State().delay_calls == 1);
    CHECK(shim::State().last_delay == 1);  // not 0: 1 ms at 100 Hz still waits a tick
    os_delay_msec(5);
    CHECK(shim::State().last_delay == 1);
    os_delay_msec(30);
    CHECK(shim::State().last_delay == 3);
    os_delay_msec(0);
    CHECK(shim::State().last_delay == 0);  // 0 ms stays a yield
}

void ShortTimeoutsWaitAtLeastOneTick() {
    shim::Reset();
    OsQueue<uint32_t, 1> q("short");
    uint32_t v = 0;
    CHECK(!q.Receive(v, 1));
    CHECK(shim::State().last_queue_wait == 1);
    CHECK(!q.Receive(v, 0));
    CHECK(shim::State().last_queue_wait == 0);  // a poll stays a poll

    OsEventFlags<> ev("short");
    uint32_t seen = 0;
    CHECK(!ev.Wait(0x1, WaitMode::Any, 3, seen));
    CHECK(shim::State().last_event_wait == 1);
}

}  // namespace

int main() {
    struct Case {
        const char* name;
        void (*fn)();
    };
    const Case cases[] = {
        {"QueueUsesInlineStorageWithItemCountLength", QueueUsesInlineStorageWithItemCountLength},
        {"QueueHoldsExactlyItsCapacity", QueueHoldsExactlyItsCapacity},
        {"QueueCopiesOnlyTheItem", QueueCopiesOnlyTheItem},
        {"QueueTimeoutIsConvertedToTicks", QueueTimeoutIsConvertedToTicks},
        {"EventWaitTimesOutWithFalse", EventWaitTimesOutWithFalse},
        {"EventWaitDoesNotClearBits", EventWaitDoesNotClearBits},
        {"FlagsSaverWaitConsumesTheChange", FlagsSaverWaitConsumesTheChange},
        {"SeqlockWaitConsumesTheChange", SeqlockWaitConsumesTheChange},
        {"ShortDelaysSleepAtLeastOneTick", ShortDelaysSleepAtLeastOneTick},
        {"ShortTimeoutsWaitAtLeastOneTick", ShortTimeoutsWaitAtLeastOneTick},
    };
    for (const Case& c : cases) {
        const int before = g_failures;
        c.fn();
        std::printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", c.name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
