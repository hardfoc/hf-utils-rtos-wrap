/**
 * @file FlagsSaver.h
 * @brief Atomic two-state flag bitset with optional event-flag waiter.
 *
 * Three types in one header:
 *   - `hf::FlagsReader<FlagId, kCount>`  — pure-virtual read-side ABC.
 *   - `hf::FlagsWriter<FlagId, kCount>`  — pure-virtual write-side ABC.
 *   - `hf::FlagsSaver<FlagId, kCount>`   — concrete impl deriving from both.
 *
 * Pass an `hf::FlagsReader*` to consumers that may only inspect / wait, and
 * an `hf::FlagsWriter*` to consumers that may only publish — keeps the
 * apps↔middleware layering honest.
 *
 * @par Storage
 *   Each slot holds one bit (0 = Cleared, 1 = Set); slots are packed into
 *   `std::atomic<uint64_t>` words (64 slots per word). Lookups and writes
 *   are lock-free. A change to any slot bumps a 32-bit sequence counter
 *   and signals a single bit on a lazily-created `OsEventFlags`.
 *
 * @par Thread-safety
 *   - Set / Clear / IsSet / Snapshot / Seq / LastChangeMs: lock-free; safe
 *     from any task context. Not ISR-safe (FreeRTOS event-group set is not
 *     ISR-safe via this path).
 *   - WaitForChange: backed by a FreeRTOS event group; do not call from ISR.
 *
 * @par Allocation
 *   No heap allocation. Storage is a fixed-size atomic word array sized at
 *   compile time. The event group is created on first use.
 */
#ifndef HF_UTILS_RTOS_WRAP_FLAGSSAVER_H_
#define HF_UTILS_RTOS_WRAP_FLAGSSAVER_H_

#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>

#include "OsAbstraction.h"
#include "OsUtility.h"

namespace hf {

/**
 * @brief Snapshot of a `FlagsSaver` at a point in time.
 *
 * Tail-template sized so a snapshot fits any specialisation; the writer
 * fills `bits[0 .. kWordCount - 1]`, leaves the rest zero.
 */
template <std::size_t kWordCount>
struct FlagsSnapshot {
    uint64_t bits[kWordCount > 0 ? kWordCount : 1]{};
    uint32_t seq{0};
    uint32_t last_change_ms{0};
};

/**
 * @brief Read-side ABC for `FlagsSaver`.
 */
template <typename FlagId, std::size_t kCount>
class FlagsReader {
public:
    static constexpr std::size_t kWordCount =
        (kCount + 63U) / 64U > 0U ? (kCount + 63U) / 64U : 1U;
    using Snapshot = FlagsSnapshot<kWordCount>;

    virtual ~FlagsReader() noexcept = default;

    /// True if @p id is currently set.
    [[nodiscard]] virtual bool IsSet(FlagId id) const noexcept = 0;

    /// Copy current bit state + sequence + last-change timestamp into @p out.
    virtual void Snapshot_(Snapshot& out) const noexcept = 0;

    /// Monotonic change counter; increments on every Set/Clear that
    /// actually toggled state.
    [[nodiscard]] virtual uint32_t Seq() const noexcept = 0;

    /// Most recent `now_ms` value passed to `Set` / `Clear` (0 if never).
    [[nodiscard]] virtual uint32_t LastChangeMs() const noexcept = 0;

    /**
     * @brief Block up to @p timeout_ms waiting for any change since the
     *        last call to `WaitForChange` or `ClearWaitEvent`.
     * @return `true` on signal; `false` on timeout / waiter unavailable.
     */
    virtual bool WaitForChange(uint32_t timeout_ms) noexcept = 0;

    /// Discard any pending change-signal without waiting.
    virtual bool ClearWaitEvent() noexcept = 0;
};

/**
 * @brief Write-side ABC for `FlagsSaver`.
 */
template <typename FlagId, std::size_t kCount>
class FlagsWriter {
public:
    virtual ~FlagsWriter() noexcept = default;

    /**
     * @brief Mark @p id set. Idempotent: returns `false` if state was
     *        already `Set` (no event signalled).
     * @param now_ms Optional timestamp recorded as `LastChangeMs()` when
     *               the bit actually transitioned. Pass 0 to skip.
     */
    virtual bool Set(FlagId id, uint32_t now_ms = 0) noexcept = 0;

    /**
     * @brief Mark @p id cleared. Idempotent: returns `false` if state was
     *        already `Cleared` (no event signalled).
     */
    virtual bool Clear(FlagId id, uint32_t now_ms = 0) noexcept = 0;

    /// Reset every slot to `Cleared` and signal a change.
    virtual bool ClearAll(uint32_t now_ms = 0) noexcept = 0;
};

/**
 * @brief Concrete two-state flag bitset.
 *
 * @tparam FlagId  Strongly-typed enum (or any integer-castable type)
 *                 indexing slots; must produce values in `[0, kCount)`.
 * @tparam kCount  Number of slots.
 */
template <typename FlagId, std::size_t kCount>
class FlagsSaver final
    : public FlagsReader<FlagId, kCount>
    , public FlagsWriter<FlagId, kCount>
{
public:
    using Base     = FlagsReader<FlagId, kCount>;
    using Snapshot = typename Base::Snapshot;
    static constexpr std::size_t kWordCount = Base::kWordCount;

    FlagsSaver() noexcept = default;

    ~FlagsSaver() override
    {
        if (event_group_created_) {
            (void)os_event_group_delete(&event_group_);
            event_group_created_ = false;
        }
    }

    FlagsSaver(const FlagsSaver&)            = delete;
    FlagsSaver& operator=(const FlagsSaver&) = delete;

    /* ── Writer surface ──────────────────────────────────────────── */

    bool Set(FlagId id, uint32_t now_ms = 0) noexcept override
    {
        return WriteSlot_(id, true, now_ms);
    }

    bool Clear(FlagId id, uint32_t now_ms = 0) noexcept override
    {
        return WriteSlot_(id, false, now_ms);
    }

    bool ClearAll(uint32_t now_ms = 0) noexcept override
    {
        bool changed = false;
        for (auto& w : words_) {
            const uint64_t prev = w.exchange(0, std::memory_order_acq_rel);
            if (prev != 0U) changed = true;
        }
        if (!changed) return false;
        if (now_ms != 0U) last_change_ms_.store(now_ms, std::memory_order_release);
        SignalChange_();
        return true;
    }

    /* ── Reader surface ──────────────────────────────────────────── */

    [[nodiscard]] bool IsSet(FlagId id) const noexcept override
    {
        const std::size_t idx = static_cast<std::size_t>(id);
        if (idx >= kCount) return false;
        const std::size_t word = idx / 64U;
        const std::size_t bit  = idx % 64U;
        return (words_[word].load(std::memory_order_acquire)
                & (uint64_t{1} << bit)) != 0U;
    }

    void Snapshot_(Snapshot& out) const noexcept override
    {
        for (std::size_t i = 0; i < kWordCount; ++i) {
            out.bits[i] = words_[i].load(std::memory_order_acquire);
        }
        out.seq            = seq_.load(std::memory_order_acquire);
        out.last_change_ms = last_change_ms_.load(std::memory_order_acquire);
    }

    [[nodiscard]] uint32_t Seq() const noexcept override
    {
        return seq_.load(std::memory_order_acquire);
    }

    [[nodiscard]] uint32_t LastChangeMs() const noexcept override
    {
        return last_change_ms_.load(std::memory_order_acquire);
    }

    bool WaitForChange(uint32_t timeout_ms) noexcept override
    {
        if (!EnsureEventGroup_()) return false;
        OS_Ulong actual = 0;
        const OS_Ulong wait = (timeout_ms == UINT32_MAX)
                                  ? static_cast<OS_Ulong>(OS_WAIT_FOREVER)
                                  : static_cast<OS_Ulong>(os_convert_msec_to_delay_ticks(timeout_ms));
        // Consume the signal: the next WaitForChange() waits for a new change.
        const OS_Uint rc = os_event_group_get(&event_group_, kEventBit_,
                                              static_cast<OS_Uint>(OS_OR) |
                                                  static_cast<OS_Uint>(OS_EVENT_CLEAR),
                                              &actual, wait);
        return (rc == OS_SUCCESS) && ((actual & kEventBit_) != 0U);
    }

    bool ClearWaitEvent() noexcept override
    {
        if (!EnsureEventGroup_()) return false;
        return os_event_group_clear(&event_group_, kEventBit_) == OS_SUCCESS;
    }

private:
    bool WriteSlot_(FlagId id, bool set, uint32_t now_ms) noexcept
    {
        const std::size_t idx = static_cast<std::size_t>(id);
        if (idx >= kCount) return false;

        const std::size_t word = idx / 64U;
        const uint64_t    mask = uint64_t{1} << (idx % 64U);
        auto&             w    = words_[word];

        const uint64_t prev = set
            ? w.fetch_or(mask,  std::memory_order_acq_rel)
            : w.fetch_and(~mask, std::memory_order_acq_rel);

        const bool was_set = (prev & mask) != 0U;
        if (was_set == set) return false;  // no transition

        if (now_ms != 0U) last_change_ms_.store(now_ms, std::memory_order_release);
        SignalChange_();
        return true;
    }

    void SignalChange_() noexcept
    {
        seq_.fetch_add(1, std::memory_order_acq_rel);
        if (EnsureEventGroup_()) {
            (void)os_event_group_set(&event_group_, kEventBit_);
        }
    }

    bool EnsureEventGroup_() noexcept
    {
        if (event_group_created_) return true;
        if (os_event_group_create(&event_group_, "FlagsSaver") == OS_SUCCESS) {
            event_group_created_ = true;
        }
        return event_group_created_;
    }

    static constexpr OS_Ulong kEventBit_ = 0x1U;

    std::atomic<uint64_t> words_[kWordCount]{};
    std::atomic<uint32_t> seq_{0};
    std::atomic<uint32_t> last_change_ms_{0};
    OS_EventGroup         event_group_{};
    bool                  event_group_created_{false};
};

}  // namespace hf

#endif /* HF_UTILS_RTOS_WRAP_FLAGSSAVER_H_ */
