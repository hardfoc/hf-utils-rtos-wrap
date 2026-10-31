/**
 * @file RtosMutex.cpp
 * @ingroup rtos-wrap
 * @brief RtosMutex and RtosSharedMutex member functions.
 *
 * Kept out of the header so a translation unit that locks a mutex calls only these functions, never the
 * RTOS's mutex API directly: the RTOS symbols stay in this object file (OsAbstraction.h selects the backend).
 */

#include "RtosMutex.h"

/* ── RtosMutex ── */

RtosMutex::RtosMutex() noexcept
    : handle_{} {
  os_recursive_mutex_create(&handle_, "RtosMtx");
}

RtosMutex::~RtosMutex() noexcept {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
  if (os_mutex_is_live(&handle_)) {
    os_recursive_mutex_delete(&handle_);
  }
#endif
}

RtosMutex::RtosMutex(RtosMutex&& other) noexcept
    : handle_(other.handle_) {
  other.handle_ = {};
}

RtosMutex& RtosMutex::operator=(RtosMutex&& other) noexcept {
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

bool RtosMutex::lock() noexcept {
  return os_recursive_mutex_get(&handle_, OS_WAIT_FOREVER) == OS_SUCCESS;
}

bool RtosMutex::try_lock() noexcept {
  return os_recursive_mutex_get(&handle_, OS_NO_WAIT) == OS_SUCCESS;
}

bool RtosMutex::try_lock_for(uint32_t timeout_ms) noexcept {
  const uint32_t ticks = RtosTime::MsToTicks(timeout_ms);
  return os_recursive_mutex_get(&handle_, ticks) == OS_SUCCESS;
}

void RtosMutex::unlock() noexcept {
  os_recursive_mutex_put(&handle_);
}

/* ── RtosSharedMutex ── */

RtosSharedMutex::RtosSharedMutex() noexcept
    : writer_mutex_{}, reader_mutex_{}, readers_(0), writer_active_(false) {
  os_mutex_create(&writer_mutex_, "RtosShWr", OS_INHERIT);
  os_mutex_create(&reader_mutex_, "RtosShRd", OS_INHERIT);
}

RtosSharedMutex::~RtosSharedMutex() noexcept {
#if !defined(HF_RTOS_NONE) && !defined(HF_MCU_FAMILY_NONE)
  if (os_mutex_is_live(&writer_mutex_)) {
    os_mutex_delete(&writer_mutex_);
  }
  if (os_mutex_is_live(&reader_mutex_)) {
    os_mutex_delete(&reader_mutex_);
  }
#endif
}

RtosSharedMutex::RtosSharedMutex(RtosSharedMutex&& other) noexcept
    : writer_mutex_(other.writer_mutex_), reader_mutex_(other.reader_mutex_),
      readers_(other.readers_.load()), writer_active_(other.writer_active_.load()) {
  other.writer_mutex_ = {};
  other.reader_mutex_ = {};
  other.readers_.store(0);
  other.writer_active_.store(false);
}

RtosSharedMutex& RtosSharedMutex::operator=(RtosSharedMutex&& other) noexcept {
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

bool RtosSharedMutex::lock() noexcept {
  if (os_mutex_get(&writer_mutex_, OS_WAIT_FOREVER) != OS_SUCCESS) {
    return false;
  }
  writer_active_.store(true);
  while (readers_.load() > 0) {
    os_thread_sleep(1);
  }
  return true;
}

bool RtosSharedMutex::try_lock() noexcept {
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

bool RtosSharedMutex::try_lock_for(uint32_t timeout_ms) noexcept {
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

void RtosSharedMutex::unlock() noexcept {
  writer_active_.store(false);
  os_mutex_put(&writer_mutex_);
}

bool RtosSharedMutex::lock_shared() noexcept {
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

bool RtosSharedMutex::try_lock_shared() noexcept {
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

bool RtosSharedMutex::try_lock_shared_for(uint32_t timeout_ms) noexcept {
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

void RtosSharedMutex::unlock_shared() noexcept {
  if (os_mutex_get(&reader_mutex_, OS_WAIT_FOREVER) == OS_SUCCESS) {
    if (readers_.load() > 0) {
      readers_--;
    }
    os_mutex_put(&reader_mutex_);
  }
}
