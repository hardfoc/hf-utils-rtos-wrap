/**
 * @file RtosMutex.h
 * @ingroup rtos-wrap
 * @brief Cross-platform RTOS mutex and synchronization primitives.
 *
 * This header provides platform-agnostic mutex, lock guard, and timing utilities
 * for use by all layers above the MCU implementations (handlers, managers, API).
 * The implementation is built on OsAbstraction.h which provides the underlying
 * RTOS-portable primitives, so this code works across any supported RTOS
 * (FreeRTOS, ThreadX future, Zephyr future) and bare-metal targets.
 *
 * This is the authoritative location for RtosMutex — handlers and managers
 * should include this file (from hf-utils-rtos-wrap) rather than reaching
 * into the internal interface-wrap layer.
 *
 * @author Nebiyu Tadesse
 * @date 2025
 *
 * @note Platform support includes ESP32 (FreeRTOS), STM32 (future), and
 *       RP2040 (future). Bare-metal no-op stubs are provided when no RTOS
 *       is selected (HF_RTOS_NONE).
 * @note RtosMutex and RtosSharedMutex members that call the RTOS are defined
 *       in src/RtosMutex.cpp, so code that locks a mutex never calls the
 *       RTOS mutex API itself. Build systems compile that file for every
 *       backend, HF_RTOS_NONE included.
 * @todo Add @copyright line once project copyright wording is finalised.
 */

#pragma once

#include "OsAbstraction.h"
#include "OsUtility.h"

#include <atomic>
#include <cstdint>

/* ═══════════════════════════════════════════════════════════════════════════
 * RtosTime — high-resolution timing utilities
 * ═══════════════════════════════════════════════════════════════════════════ */

class RtosTime {
public:
  /**
   * @brief Get current time in microseconds.
   * @return Current time in microseconds from the RTOS tick counter.
   */
  static uint64_t GetCurrentTimeUs() noexcept {
#if defined(HF_RTOS_NONE) || defined(HF_MCU_FAMILY_NONE)
    return 0;
#else
    const uint32_t ticks = static_cast<uint32_t>(os_time_get());
    return static_cast<uint64_t>(os_convert_delay_ticks_to_msec(ticks)) * 1000ULL;
#endif
  }

  /**
   * @brief Convert milliseconds to RTOS ticks.
   * @param ms Time in milliseconds.
   * @return Equivalent tick count (minimum 1 tick for non-zero ms).
   */
  static uint32_t MsToTicks(uint32_t ms) noexcept {
    if (ms == 0) {
      return 0;
    }
    const uint32_t ticks = os_convert_msec_to_delay_ticks(ms);
    return (ticks > 0) ? ticks : 1;
  }
};

/* ═══════════════════════════════════════════════════════════════════════════
 * RtosMutex — recursive mutex built on OsAbstraction
 * ═══════════════════════════════════════════════════════════════════════════ */

class RtosMutex {
public:
  RtosMutex() noexcept;

  ~RtosMutex() noexcept;

  RtosMutex(const RtosMutex&) = delete;
  RtosMutex& operator=(const RtosMutex&) = delete;

  RtosMutex(RtosMutex&& other) noexcept;

  RtosMutex& operator=(RtosMutex&& other) noexcept;

  /**
   * @brief Lock the mutex, blocking indefinitely.
   * @return true if successfully locked.
   */
  bool lock() noexcept;

  /**
   * @brief Try to lock the mutex without blocking.
   * @return true if successfully locked.
   */
  bool try_lock() noexcept;

  /**
   * @brief Try to lock the mutex with a timeout.
   * @param timeout_ms Maximum time to wait in milliseconds.
   * @return true if successfully locked within the timeout.
   */
  bool try_lock_for(uint32_t timeout_ms) noexcept;

  /**
   * @brief Unlock the mutex.
   */
  void unlock() noexcept;

  /**
   * @brief Get the native RTOS handle (platform-specific).
   * @return The underlying OS_Mutex handle.
   */
  OS_Mutex native_handle() const noexcept {
    return handle_;
  }

  // ── Convenience FreeRTOS-style API ──────────────────────────────────────

  /**
   * @brief Take (lock) the mutex with optional timeout.
   * @param timeout_ms Timeout in milliseconds (0 = block forever).
   * @return true if successfully taken.
   */
  bool Take(uint32_t timeout_ms = 0) noexcept {
    if (timeout_ms > 0) {
      return try_lock_for(timeout_ms);
    }
    return lock();
  }

  /**
   * @brief Give (unlock) the mutex.
   */
  void Give() noexcept {
    unlock();
  }

  // ── Shared lock methods (delegated to regular mutex for simplicity) ─────

  bool lock_shared() noexcept { return lock(); }
  bool try_lock_shared() noexcept { return try_lock(); }
  bool try_lock_shared_for(uint32_t timeout_ms) noexcept { return try_lock_for(timeout_ms); }
  void unlock_shared() noexcept { unlock(); }

private:
  OS_Mutex handle_;
};

/* ═══════════════════════════════════════════════════════════════════════════
 * RtosSharedMutex — reader-writer mutex
 *
 * Provides shared (read) and exclusive (write) locking semantics.
 * Uses two underlying OS mutexes for writer exclusion and reader counting.
 * ═══════════════════════════════════════════════════════════════════════════ */

class RtosSharedMutex {
public:
  RtosSharedMutex() noexcept;

  ~RtosSharedMutex() noexcept;

  RtosSharedMutex(const RtosSharedMutex&) = delete;
  RtosSharedMutex& operator=(const RtosSharedMutex&) = delete;

  RtosSharedMutex(RtosSharedMutex&& other) noexcept;

  RtosSharedMutex& operator=(RtosSharedMutex&& other) noexcept;

  bool lock() noexcept;

  bool try_lock() noexcept;

  bool try_lock_for(uint32_t timeout_ms) noexcept;

  void unlock() noexcept;

  bool lock_shared() noexcept;

  bool try_lock_shared() noexcept;

  bool try_lock_shared_for(uint32_t timeout_ms) noexcept;

  void unlock_shared() noexcept;

private:
  OS_Mutex writer_mutex_;
  OS_Mutex reader_mutex_;
  std::atomic<int> readers_;
  std::atomic<bool> writer_active_;
};

/* ═══════════════════════════════════════════════════════════════════════════
 * RtosUniqueLock — RAII exclusive lock (similar to std::unique_lock)
 * ═══════════════════════════════════════════════════════════════════════════ */

template <typename MutexT>
class RtosUniqueLock {
public:
  /**
   * @brief Construct and lock the mutex.
   * @param mutex Reference to the mutex to lock.
   * @param timeout_ms Timeout in ms (0 = block forever).
   */
  explicit RtosUniqueLock(MutexT& mutex, uint32_t timeout_ms = 0) noexcept
      : mutex_(&mutex), locked_(false) {
    if (timeout_ms > 0) {
      locked_ = mutex_->try_lock_for(timeout_ms);
    } else {
      locked_ = mutex_->lock();
    }
  }

  ~RtosUniqueLock() noexcept {
    if (locked_ && mutex_) {
      mutex_->unlock();
    }
  }

  RtosUniqueLock(const RtosUniqueLock&) = delete;
  RtosUniqueLock& operator=(const RtosUniqueLock&) = delete;

  RtosUniqueLock(RtosUniqueLock&& other) noexcept : mutex_(other.mutex_), locked_(other.locked_) {
    other.mutex_ = nullptr;
    other.locked_ = false;
  }

  RtosUniqueLock& operator=(RtosUniqueLock&& other) noexcept {
    if (this != &other) {
      if (locked_ && mutex_) {
        mutex_->unlock();
      }
      mutex_ = other.mutex_;
      locked_ = other.locked_;
      other.mutex_ = nullptr;
      other.locked_ = false;
    }
    return *this;
  }

  /** @brief Check if the mutex was successfully locked. */
  [[nodiscard]] bool IsLocked() const noexcept {
    return locked_;
  }

  /** @brief Manually unlock before destruction. */
  void Unlock() noexcept {
    if (locked_ && mutex_) {
      mutex_->unlock();
      locked_ = false;
    }
  }

private:
  MutexT* mutex_;
  bool locked_;
};

/* ═══════════════════════════════════════════════════════════════════════════
 * RtosSharedLock — RAII shared (reader) lock
 * ═══════════════════════════════════════════════════════════════════════════ */

template <typename SharedMutexT>
class RtosSharedLock {
public:
  explicit RtosSharedLock(SharedMutexT& mutex, uint32_t timeout_ms = 0) noexcept
      : mutex_(&mutex), locked_(false) {
    if (timeout_ms > 0) {
      locked_ = mutex_->try_lock_shared_for(timeout_ms);
    } else {
      locked_ = mutex_->lock_shared();
    }
  }

  ~RtosSharedLock() noexcept {
    if (locked_ && mutex_) {
      mutex_->unlock_shared();
    }
  }

  RtosSharedLock(const RtosSharedLock&) = delete;
  RtosSharedLock& operator=(const RtosSharedLock&) = delete;

  RtosSharedLock(RtosSharedLock&& other) noexcept : mutex_(other.mutex_), locked_(other.locked_) {
    other.mutex_ = nullptr;
    other.locked_ = false;
  }

  RtosSharedLock& operator=(RtosSharedLock&& other) noexcept {
    if (this != &other) {
      if (locked_ && mutex_) {
        mutex_->unlock_shared();
      }
      mutex_ = other.mutex_;
      locked_ = other.locked_;
      other.mutex_ = nullptr;
      other.locked_ = false;
    }
    return *this;
  }

  [[nodiscard]] bool IsLocked() const noexcept {
    return locked_;
  }

  void Unlock() noexcept {
    if (locked_ && mutex_) {
      mutex_->unlock_shared();
      locked_ = false;
    }
  }

private:
  SharedMutexT* mutex_;
  bool locked_;
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Convenience Type Aliases
 * ═══════════════════════════════════════════════════════════════════════════ */

/// @brief Convenience alias for unique lock guard
template <typename MutexT>
using RtosLockGuard = RtosUniqueLock<MutexT>;

/// @brief Convenience alias for RtosMutex lock guard
using MutexLockGuard = RtosUniqueLock<RtosMutex>;
