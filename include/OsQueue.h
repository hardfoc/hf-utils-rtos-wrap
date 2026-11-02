/**
 * @file OsQueue.h
 * @brief Lightweight C++ wrapper for the RTOS queue primitive.
 *
 * Thread-safety: internally thread-safe; the underlying FreeRTOS queue is
 * already MT-safe so no extra mutex is layered on top.
 *
 * Allocation: queue storage is held inline as a member array sized by the
 * backend's OS_QUEUE_STORE_BYTES(); on FreeRTOS (with static allocation) and
 * ThreadX the kernel queue lives there and nothing is taken from the heap.
 * CMSIS-RTOS2 allocates the queue in the kernel. The underlying RTOS queue is
 * created eagerly in the constructor.
 *
 * Items travel through the kernel as whole 32-bit words: Send() and Receive()
 * copy through a word buffer, so a `MessageType` whose size is not a multiple
 * of 4 is never read or written past its end.
 *
 * Public surface uses modern C++ types — `uint32_t timeout_ms`, `bool`. No
 * `OS_Ulong` / `OS_WAIT_FOREVER` leakage. Pass `UINT32_MAX` to wait forever.
 */
#ifndef OS_QUEUE_H_
#define OS_QUEUE_H_

#include <cstdint>
#include <cstddef>
#include <climits>
#include <cstring>
#include <type_traits>
#include "OsAbstraction.h"
#include "OsUtility.h"

/**
 * @class OsQueue
 * @brief Fixed-capacity, eager-init MPMC queue of trivially-copyable items.
 *
 * @tparam MessageType  Element type (must be trivially copyable; passed by
 *                      value through the underlying queue).
 * @tparam kCapacity    Maximum number of pending elements.
 */
template <typename MessageType, size_t kCapacity>
class OsQueue {
    static_assert(std::is_trivially_copyable_v<MessageType>,
                  "OsQueue<T, N> requires a trivially copyable T");
    static_assert(kCapacity > 0, "OsQueue<T, N> needs a capacity of at least 1");

public:
    /**
     * @brief Construct and create the underlying RTOS queue eagerly.
     */
    explicit OsQueue(const char* queueName) noexcept
        : name_(queueName)
    {
        created_ = os_queue_create_ex(queue_, name_, static_cast<OS_Uint>(kItemWords),
                                      storage_, static_cast<OS_Ulong>(kCapacity));
    }

    OsQueue(const OsQueue&) = delete;
    OsQueue& operator=(const OsQueue&) = delete;

    ~OsQueue() noexcept
    {
        if (created_) {
            os_queue_delete_ex(queue_);
        }
    }

    /// True if the underlying RTOS queue was created successfully.
    [[nodiscard]] bool IsValid() const noexcept { return created_; }

    /**
     * @brief Send a message; blocks up to @p timeout_ms.
     * @return true on success, false on timeout / failure.
     */
    bool Send(const MessageType& message, uint32_t timeout_ms = UINT32_MAX) noexcept
    {
        if (!created_) return false;
        uint32_t item[kItemWords] = {};
        std::memcpy(item, &message, sizeof(MessageType));
        return os_queue_send_ex(queue_, item, ToTicks(timeout_ms));
    }

    /**
     * @brief Receive a message; blocks up to @p timeout_ms.
     * @return true on success, false on timeout / failure.
     */
    bool Receive(MessageType& out, uint32_t timeout_ms = UINT32_MAX) noexcept
    {
        if (!created_) return false;
        uint32_t item[kItemWords] = {};
        if (!os_queue_receive_ex(queue_, item, ToTicks(timeout_ms))) return false;
        std::memcpy(&out, item, sizeof(MessageType));
        return true;
    }

    /**
     * @brief Compile-time capacity in elements.
     */
    [[nodiscard]] static constexpr size_t Capacity() noexcept { return kCapacity; }

private:
    /// Item size in 32-bit words, as the kernel queue stores it.
    static constexpr size_t kItemWords = (sizeof(MessageType) + sizeof(uint32_t) - 1U) / sizeof(uint32_t);
    /// Backing storage the backend asks for (at least one byte, so the array is never empty).
    static constexpr size_t kStoreBytes =
        (OS_QUEUE_STORE_BYTES(kItemWords, kCapacity) > 0U) ? OS_QUEUE_STORE_BYTES(kItemWords, kCapacity) : 1U;

    static OS_Ulong ToTicks(uint32_t timeout_ms) noexcept
    {
        return (timeout_ms == UINT32_MAX)
            ? static_cast<OS_Ulong>(OS_WAIT_FOREVER)
            : static_cast<OS_Ulong>(os_convert_msec_to_delay_ticks(timeout_ms));
    }

    const char* name_;
    bool        created_{false};
    OS_Queue    queue_{};
    alignas(std::max_align_t) uint8_t storage_[kStoreBytes];
};

#endif /* OS_QUEUE_H_ */

