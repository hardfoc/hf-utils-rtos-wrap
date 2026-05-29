#include "BaseThread.h"

#if defined(HF_RTOS_NONE)

bool BaseThread::StartThreadAndWaitToVerify(uint32_t /*startTimeoutMsec*/) {
  if (!EnsureInitialized()) return false;
  if (!IsSetupComplete()) {
    Setup();
    MarkSetupComplete();
  }
  MarkThreadRunning();
  return true;
}

bool BaseThread::StopThreadAndWaitToVerify(uint32_t /*stopTimeoutMsec*/) {
  MarkThreadStopRequested();
  ClearThreadRunning();
  return true;
}

uint32_t BaseThread::GetStackHighWaterMark() const noexcept { return 0; }

bool BaseThread::ChangePriority(uint32_t /*newPriority*/) noexcept { return true; }

#endif
