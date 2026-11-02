/**
 * @file SeqlockSnapshot.h
 * @brief Single-writer / many-reader coherent snapshot of a POD value.
 *
 * Three types in one header:
 *   - `hf::SnapshotReader<T>`   — pure-virtual read-side ABC.
 *   - `hf::SnapshotWriter<T>`   — pure-virtual write-side ABC.
 *   - `hf::SeqlockSnapshot<T>`  — concrete impl deriving from both.
 *
 * @par Algorithm
 *   Writer: bumps `seq_` to odd → memcpy `T` payload → bumps `seq_` to even.
 *   Reader: snapshots `seq_` → memcpy payload → re-reads `seq_`; retry while
 *   the start seq was odd or the two reads differ.
 *
 * @par Thread-safety
 *   - Single writer assumed (`Publish` not safe to call concurrently from
 *     multiple writers; protect externally if ever needed).
 *   - Many readers — `Read` is wait-free in the absence of writes and
 *     bounded-retry under contention.
 *   - Optional waiter: backed by a lazily-created FreeRTOS event group;
 *     `WaitForChange` is task-context only.
 *
 * @par Allocation
 *   No heap allocation. Inline `T` storage; event group created on first
 *   `WaitForChange` / `ClearWaitEvent` use.
 *
 * @par Constraints
 *   `T` must be trivially copyable.
 */
#ifndef HF_UTILS_RTOS_WRAP_SEQLOCKSNAPSHOT_H_
#define HF_UTILS_RTOS_WRAP_SEQLOCKSNAPSHOT_H_

#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "OsAbstraction.h"
#include "OsUtility.h"

namespace hf {

template <typename T>
class SnapshotReader {
public:
    virtual ~SnapshotReader() noexcept = default;

    /**
     * @brief Read the latest coherent snapshot.
     * @return current sequence (post-read); always even on success.
     */
    virtual uint32_t Read(T& out) const noexcept = 0;

    /// Monotonic change counter (each successful `Publish` adds 2).
    [[nodiscard]] virtual uint32_t Seq() const noexcept = 0;

    /**
     * @brief Block up to @p timeout_ms waiting for any change since the
     *        last call to `WaitForChange` / `ClearWaitEvent`.
     */
    virtual bool WaitForChange(uint32_t timeout_ms) noexcept = 0;

    /// Drop any pending change-signal without waiting.
    virtual bool ClearWaitEvent() noexcept = 0;
};

template <typename T>
class SnapshotWriter {
public:
    virtual ~SnapshotWriter() noexcept = default;

    /**
     * @brief Atomically replace the published snapshot with @p value.
     *
     * Single-writer only. Increments `Seq()` by 2 and signals waiters.
     */
    virtual void Publish(const T& value) noexcept = 0;
};

template <typename T>
class SeqlockSnapshot final
    : public SnapshotReader<T>
    , public SnapshotWriter<T>
{
    static_assert(std::is_trivially_copyable_v<T>,
                  "SeqlockSnapshot<T> requires a trivially copyable T");

public:
    SeqlockSnapshot() noexcept = default;

    ~SeqlockSnapshot() override
    {
        if (event_group_created_) {
            (void)os_event_group_delete(&event_group_);
            event_group_created_ = false;
        }
    }

    SeqlockSnapshot(const SeqlockSnapshot&)            = delete;
    SeqlockSnapshot& operator=(const SeqlockSnapshot&) = delete;

    /* ── Writer ──────────────────────────────────────────────────── */

    void Publish(const T& value) noexcept override
    {
        const uint32_t s0 = seq_.load(std::memory_order_relaxed);
        seq_.store(s0 + 1U, std::memory_order_release);   // mark odd (writing)
        std::atomic_thread_fence(std::memory_order_release);
        std::memcpy(&payload_, &value, sizeof(T));
        std::atomic_thread_fence(std::memory_order_release);
        seq_.store(s0 + 2U, std::memory_order_release);   // back to even
        SignalChange_();
    }

    /* ── Reader ──────────────────────────────────────────────────── */

    uint32_t Read(T& out) const noexcept override
    {
        for (;;) {
            const uint32_t s1 = seq_.load(std::memory_order_acquire);
            if ((s1 & 1U) != 0U) {
                continue;  // writer in progress; spin
            }
            std::atomic_thread_fence(std::memory_order_acquire);
            std::memcpy(&out, &payload_, sizeof(T));
            std::atomic_thread_fence(std::memory_order_acquire);
            const uint32_t s2 = seq_.load(std::memory_order_acquire);
            if (s1 == s2) {
                return s2;
            }
            // torn read — retry
        }
    }

    [[nodiscard]] uint32_t Seq() const noexcept override
    {
        return seq_.load(std::memory_order_acquire);
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
    void SignalChange_() noexcept
    {
        if (EnsureEventGroup_()) {
            (void)os_event_group_set(&event_group_, kEventBit_);
        }
    }

    bool EnsureEventGroup_() noexcept
    {
        if (event_group_created_) return true;
        if (os_event_group_create(&event_group_, "SeqlockSnap") == OS_SUCCESS) {
            event_group_created_ = true;
        }
        return event_group_created_;
    }

    static constexpr OS_Ulong kEventBit_ = 0x1U;

    mutable std::atomic<uint32_t> seq_{0};
    T                             payload_{};
    OS_EventGroup                 event_group_{};
    bool                          event_group_created_{false};
};

}  // namespace hf

#endif /* HF_UTILS_RTOS_WRAP_SEQLOCKSNAPSHOT_H_ */
