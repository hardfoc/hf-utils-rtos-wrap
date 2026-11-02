/*
 * Host stand-in for the FreeRTOS kernel headers, for tests/host only.
 *
 * Single-threaded: a blocking call never blocks. A queue send or receive that
 * cannot complete, or an event-group wait whose condition is not met, returns
 * at once as if its timeout had expired. Queues and event groups are modelled;
 * every call records the tick count it was given (see freertos_shim.h). Tasks,
 * timers, semaphores and stream buffers are declared so the backend compiles,
 * and fail or do nothing.
 */
#ifndef HF_TEST_FREERTOS_SHIM_H_
#define HF_TEST_FREERTOS_SHIM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;
typedef TickType_t EventBits_t;

#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdFAIL  pdFALSE
#define pdPASS  pdTRUE
#define errQUEUE_EMPTY ((BaseType_t)0)
#define errQUEUE_FULL  ((BaseType_t)0)
#define errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY (-1)
#define errQUEUE_BLOCKED (-4)
#define errQUEUE_YIELD   (-5)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)

/* A 100 Hz tick, so a millisecond timeout that reaches the kernel unconverted is visible. */
#define configTICK_RATE_HZ ((TickType_t)100)
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1

typedef struct hf_shim_task* TaskHandle_t;
typedef struct hf_shim_queue* QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;
typedef struct hf_shim_event_group* EventGroupHandle_t;
typedef struct hf_shim_timer* TimerHandle_t;
typedef struct hf_shim_stream* StreamBufferHandle_t;
typedef void (*TaskFunction_t)(void*);
typedef void (*TimerCallbackFunction_t)(TimerHandle_t);

/* Same size and alignment as the kernel's opaque StaticQueue_t on a 64-bit host. */
typedef struct { void* dummy[10]; UBaseType_t dummy2[4]; } StaticQueue_t;

typedef enum { eRunning = 0, eReady, eBlocked, eSuspended, eDeleted, eInvalid } eTaskState;
#define taskSCHEDULER_NOT_STARTED ((BaseType_t)1)
#define taskSCHEDULER_RUNNING     ((BaseType_t)2)

#define portENTER_CRITICAL() ((void)0)
#define portEXIT_CRITICAL()  ((void)0)
#define taskYIELD()          ((void)0)

void* pvPortMalloc(size_t size);
void vPortFree(void* p);

BaseType_t xTaskCreate(TaskFunction_t fn, const char* name, uint32_t depth, void* arg,
                       UBaseType_t prio, TaskHandle_t* out);
void vTaskSuspend(TaskHandle_t t);
void vTaskResume(TaskHandle_t t);
void vTaskDelete(TaskHandle_t t);
void vTaskDelay(TickType_t ticks);
eTaskState eTaskGetState(TaskHandle_t t);
BaseType_t xTaskGetSchedulerState(void);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
TickType_t xTaskGetTickCount(void);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t t);
void vTaskPrioritySet(TaskHandle_t t, UBaseType_t prio);

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
QueueHandle_t xQueueCreateStatic(UBaseType_t length, UBaseType_t item_size, uint8_t* storage,
                                 StaticQueue_t* control);
void vQueueDelete(QueueHandle_t q);
BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t wait);
BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t wait);

SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max, UBaseType_t initial);
BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t wait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t s);
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t s, TickType_t wait);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t s);
void vSemaphoreDelete(SemaphoreHandle_t s);
UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t s);

EventGroupHandle_t xEventGroupCreate(void);
void vEventGroupDelete(EventGroupHandle_t g);
EventBits_t xEventGroupSetBits(EventGroupHandle_t g, EventBits_t bits);
EventBits_t xEventGroupClearBits(EventGroupHandle_t g, EventBits_t bits);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t g, EventBits_t bits, BaseType_t clear_on_exit,
                                BaseType_t wait_for_all, TickType_t wait);

TimerHandle_t xTimerCreate(const char* name, TickType_t period, UBaseType_t reload, void* id,
                           TimerCallbackFunction_t cb);
void* pvTimerGetTimerID(TimerHandle_t t);
BaseType_t xTimerStart(TimerHandle_t t, TickType_t wait);
BaseType_t xTimerStop(TimerHandle_t t, TickType_t wait);
BaseType_t xTimerDelete(TimerHandle_t t, TickType_t wait);

StreamBufferHandle_t xStreamBufferCreate(size_t size, size_t trigger);
void vStreamBufferDelete(StreamBufferHandle_t b);
size_t xStreamBufferSend(StreamBufferHandle_t b, const void* data, size_t len, TickType_t wait);
size_t xStreamBufferReceive(StreamBufferHandle_t b, void* data, size_t len, TickType_t wait);

#ifdef __cplusplus
}
#endif

#endif /* HF_TEST_FREERTOS_SHIM_H_ */
