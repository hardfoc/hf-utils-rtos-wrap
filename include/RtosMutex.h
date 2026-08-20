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
  RtosMutex() noexcept : handle_{} {
    os_recursive_mutex_create(&handle_, "RtosMtx");
  }

  ~RtosMutex() noexcept {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
    if (os_mutex_is_live(&handle_)) {
      os_recursive_mutex_delete(&handle_);
    }
#endif
  }

  RtosMutex(const RtosMutex&) = delete;
  RtosMutex& operator=(const RtosMutex&) = delete;

  RtosMutex(RtosMutex&& other) noexcept : handle_(other.handle_) {
    other.handle_ = {};
  }

  RtosMutex& operator=(RtosMutex&& other) noexcept {
    if (this != &other) {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
      if (os_mutex_is_live(&handle_)) {
        os_recursive_mutex_delete(&handle_);
      }
#endif
      handle_ = other.handle_;
      other.handle_ = {};
    }
    return *this;
  }

  /**
   * @brief Lock the mutex, blocking indefinitely.
   * @return true if successfully locked.
   */
  bool lock() noexcept {
    return os_recursive_mutex_get(&handle_, OS_WAIT_FOREVER) == OS_SUCCESS;
  }

  /**
   * @brief Try to lock the mutex without blocking.
   * @return true if successfully locked.
   */
  bool try_lock() noexcept {
    return os_recursive_mutex_get(&handle_, OS_NO_WAIT) == OS_SUCCESS;
  }

  /**
   * @brief Try to lock the mutex with a timeout.
   * @param timeout_ms Maximum time to wait in milliseconds.
   * @return true if successfully locked within the timeout.
   */
  bool try_lock_for(uint32_t timeout_ms) noexcept {
    const uint32_t ticks = RtosTime::MsToTicks(timeout_ms);
    return os_recursive_mutex_get(&handle_, ticks) == OS_SUCCESS;
  }

  /**
   * @brief Unlock the mutex.
   */
  void unlock() noexcept {
    os_recursive_mutex_put(&handle_);
  }

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
  RtosSharedMutex() noexcept : writer_mutex_{}, reader_mutex_{}, readers_(0), writer_active_(false) {
    os_mutex_create(&writer_mutex_, "RtosShWr", OS_INHERIT);
    os_mutex_create(&reader_mutex_, "RtosShRd", OS_INHERIT);
  }

  ~RtosSharedMutex() noexcept {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
    if (os_mutex_is_live(&writer_mutex_)) {
      os_mutex_delete(&writer_mutex_);
    }
    if (os_mutex_is_live(&reader_mutex_)) {
      os_mutex_delete(&reader_mutex_);
    }
#endif
  }

  RtosSharedMutex(const RtosSharedMutex&) = delete;
  RtosSharedMutex& operator=(const RtosSharedMutex&) = delete;

  RtosSharedMutex(RtosSharedMutex&& other) noexcept
      : writer_mutex_(other.writer_mutex_), reader_mutex_(other.reader_mutex_),
        readers_(other.readers_.load()), writer_active_(other.writer_active_.load()) {
    other.writer_mutex_ = {};
    other.reader_mutex_ = {};
    other.readers_.store(0);
    other.writer_active_.store(false);
  }

  RtosSharedMutex& operator=(RtosSharedMutex&& other) noexcept {
    if (this != &other) {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
      if (os_mutex_is_live(&writer_mutex_)) { os_mutex_delete(&writer_mutex_); }
      if (os_mutex_is_live(&reader_mutex_)) { os_mutex_delete(&reader_mutex_); }
#endif
      writer_mutex_ = other.writer_mutex_;
      reader_mutex_ = other.reader_mutex_;
      readers_.store(other.readers_.load());
      writer_active_.store(other.writer_active_.load());
      other.writer_mutex_ = {};
      other.reader_mutex_ = {};
      other.readers_.store(0);
      other.writer_active_.store(false);
    }
    return *this;
  }

  bool lock() noexcept {
    if (os_mutex_get(&writer_mutex_, OS_WAIT_FOREVER) != OS_SUCCESS) {
      return false;
    }
    writer_active_.store(true);
    while (readers_.load() > 0) {
      os_thread_sleep(1);
    }
    return true;
  }

  bool try_lock() noexcept {
    if (os_mutex_get(&writer_mutex_, OS_NO_WAIT) != OS_SUCCESS) {
      return false;
    }
    writer_active_.store(true);
    if (readers_.load() > 0) {
      writer_active_.store(false);
      os_mutex_put(&writer_mutex_);
      return false;
    }
    return true;
  }

  bool try_lock_for(uint32_t timeout_ms) noexcept {
    const uint32_t ticks = RtosTime::MsToTicks(timeout_ms);
    if (os_mutex_get(&writer_mutex_, ticks) != OS_SUCCESS) {
      return false;
    }
    writer_active_.store(true);
    // Spin-wait for readers with remaining timeout (simplified)
    const uint32_t end_tick = static_cast<uint32_t>(os_time_get()) + ticks;
    while (readers_.load() > 0) {
      if (static_cast<uint32_t>(os_time_get()) >= end_tick) {
        writer_active_.store(false);
        os_mutex_put(&writer_mutex_);
        return false;
      }
      os_thread_sleep(1);
    }
    return true;
  }

  void unlock() noexcept {
    writer_active_.store(false);
    os_mutex_put(&writer_mutex_);
  }

  bool lock_shared() noexcept {
    while (true) {
      if (os_mutex_get(&reader_mutex_, OS_WAIT_FOREVER) != OS_SUCCESS) {
        return false;
      }
      if (!writer_active_.load()) {
        readers_++;
        os_mutex_put(&reader_mutex_);
        return true;
      }
      os_mutex_put(&reader_mutex_);
      os_thread_sleep(1);
    }
  }

  bool try_lock_shared() noexcept {
    if (os_mutex_get(&reader_mutex_, OS_NO_WAIT) != OS_SUCCESS) {
      return false;
    }
    if (!writer_active_.load()) {
      readers_++;
      os_mutex_put(&reader_mutex_);
      return true;
    }
    os_mutex_put(&reader_mutex_);
    return false;
  }

  bool try_lock_shared_for(uint32_t timeout_ms) noexcept {
    const uint32_t ticks = RtosTime::MsToTicks(timeout_ms);
    const uint32_t start_time = static_cast<uint32_t>(os_time_get());
    while (true) {
      const uint32_t elapsed = static_cast<uint32_t>(os_time_get()) - start_time;
      if (elapsed >= ticks) {
        return false;
      }
      const uint32_t remaining = ticks - elapsed;
      if (os_mutex_get(&reader_mutex_, remaining) != OS_SUCCESS) {
        return false;
      }
      if (!writer_active_.load()) {
        readers_++;
        os_mutex_put(&reader_mutex_);
        return true;
      }
      os_mutex_put(&reader_mutex_);
      const uint32_t new_elapsed = static_cast<uint32_t>(os_time_get()) - start_time;
      if (new_elapsed >= ticks) {
        return false;
      }
      os_thread_sleep(1);
    }
  }

  void unlock_shared() noexcept {
    if (os_mutex_get(&reader_mutex_, OS_WAIT_FOREVER) == OS_SUCCESS) {
      if (readers_.load() > 0) {
        readers_--;
      }
      os_mutex_put(&reader_mutex_);
    }
  }

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
