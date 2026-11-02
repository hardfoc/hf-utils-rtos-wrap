/*
 * Declaration-only stand-in for CMSIS-RTOS2's cmsis_os2.h: the types, constants
 * and functions that include/os/os_cmsis.h uses, with the API's signatures.
 * tests/host/run.sh compiles the library against it to check that the
 * HF_RTOS_CMSIS build compiles; nothing is linked or run.
 */
#ifndef HF_TEST_CMSIS_OS2_H_
#define HF_TEST_CMSIS_OS2_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { osOK = 0, osError = -1, osErrorTimeout = -2, osErrorResource = -3,
               osErrorParameter = -4, osErrorNoMemory = -5, osErrorISR = -6 } osStatus_t;
typedef enum { osKernelInactive = 0, osKernelReady = 1, osKernelRunning = 2,
               osKernelLocked = 3, osKernelSuspended = 4, osKernelError = -1 } osKernelState_t;
typedef enum { osThreadInactive = 0, osThreadReady = 1, osThreadRunning = 2, osThreadBlocked = 3,
               osThreadTerminated = 4, osThreadError = -1 } osThreadState_t;
typedef enum { osPriorityNone = 0, osPriorityIdle = 1, osPriorityLow = 8, osPriorityNormal = 24,
               osPriorityHigh = 40, osPriorityRealtime = 48, osPriorityISR = 56,
               osPriorityError = -1 } osPriority_t;
typedef enum { osTimerOnce = 0, osTimerPeriodic = 1 } osTimerType_t;

#define osWaitForever   0xFFFFFFFFU
#define osFlagsWaitAny  0x00000000U
#define osFlagsWaitAll  0x00000001U
#define osFlagsNoClear  0x00000002U
#define osFlagsError    0x80000000U
#define osMutexRecursive    0x00000001U
#define osMutexPrioInherit  0x00000002U

typedef void* osThreadId_t;
typedef void* osTimerId_t;
typedef void* osEventFlagsId_t;
typedef void* osMutexId_t;
typedef void* osSemaphoreId_t;
typedef void* osMessageQueueId_t;
typedef void (*osThreadFunc_t)(void* argument);
typedef void (*osTimerFunc_t)(void* argument);

typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; void* stack_mem;
                 uint32_t stack_size; osPriority_t priority; uint32_t tz_module; uint32_t reserved; } osThreadAttr_t;
typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; } osTimerAttr_t;
typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; } osEventFlagsAttr_t;
typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; } osMutexAttr_t;
typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; } osSemaphoreAttr_t;
typedef struct { const char* name; uint32_t attr_bits; void* cb_mem; uint32_t cb_size; void* mq_mem;
                 uint32_t mq_size; } osMessageQueueAttr_t;

osKernelState_t osKernelGetState(void);
int32_t osKernelLock(void);
int32_t osKernelUnlock(void);
uint32_t osKernelGetTickCount(void);
uint32_t osKernelGetTickFreq(void);

osThreadId_t osThreadNew(osThreadFunc_t func, void* argument, const osThreadAttr_t* attr);
osThreadId_t osThreadGetId(void);
osThreadState_t osThreadGetState(osThreadId_t thread_id);
osStatus_t osThreadYield(void);
osStatus_t osThreadSuspend(osThreadId_t thread_id);
osStatus_t osThreadResume(osThreadId_t thread_id);
osStatus_t osThreadTerminate(osThreadId_t thread_id);
osStatus_t osDelay(uint32_t ticks);

osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void* argument, const osTimerAttr_t* attr);
osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks);
osStatus_t osTimerStop(osTimerId_t timer_id);
osStatus_t osTimerDelete(osTimerId_t timer_id);

osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t* attr);
uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags);
uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags);
uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout);
osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id);

osMutexId_t osMutexNew(const osMutexAttr_t* attr);
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t osMutexRelease(osMutexId_t mutex_id);
osStatus_t osMutexDelete(osMutexId_t mutex_id);

osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t* attr);
osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout);
osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id);
uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id);
osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id);

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t* attr);
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void* msg_ptr, uint8_t msg_prio, uint32_t timeout);
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void* msg_ptr, uint8_t* msg_prio, uint32_t timeout);
osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id);

#ifdef __cplusplus
}
#endif

#endif /* HF_TEST_CMSIS_OS2_H_ */
