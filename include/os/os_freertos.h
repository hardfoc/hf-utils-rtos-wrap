#pragma once
/**
 * @file os_freertos.h
 * @brief FreeRTOS backend for the generic OS abstraction.
 *
 * Included only by @ref OsAbstraction.h when `HF_RTOS_FREERTOS` is selected.
 * Do not include this header directly.
 *
 * ## Supported flavors
 *
 * | Flavor | Kernel headers | Task affinity | Critical section |
 * |--------|----------------|---------------|------------------|
 * | Vanilla (CubeMX, ARM_CMx ports) | flat `FreeRTOS.h` | single core | ISR masking |
 * | ESP-IDF SMP | `freertos/FreeRTOS.h` | pinned-to-core | spinlock (`portMUX_TYPE`) |
 *
 * Both are detected here, at the port layer. A consumer build must **not**
 * force-include a compatibility header or synthesize a `freertos/` include
 * directory to satisfy this backend.
 *
 * @see os/README.md
 */

/* ── Kernel header layout ─────────────────────────────────────────────────
 * ESP-IDF installs the kernel under a `freertos/` prefix; every other port
 * (CubeMX, vanilla upstream, Zephyr's FreeRTOS shim) ships the headers flat.
 * Probe instead of assuming, so neither layout needs a build-system shim.  */
#if defined(__has_include)
#  if __has_include("freertos/FreeRTOS.h")
#    define HF_OS_FREERTOS_PREFIXED 1
#  else
#    define HF_OS_FREERTOS_PREFIXED 0
#  endif
#elif defined(ESP_PLATFORM)
#  define HF_OS_FREERTOS_PREFIXED 1
#else
#  define HF_OS_FREERTOS_PREFIXED 0
#endif

#if HF_OS_FREERTOS_PREFIXED
#  include "freertos/FreeRTOS.h"
#  include "freertos/task.h"
#  include "freertos/semphr.h"
#  include "freertos/queue.h"
#  include "freertos/event_groups.h"
#  include "freertos/timers.h"
#  include "freertos/stream_buffer.h"
#else
#  include "FreeRTOS.h"
#  include "task.h"
#  include "semphr.h"
#  include "queue.h"
#  include "event_groups.h"
#  include "timers.h"
#  include "stream_buffer.h"
#endif

#include <stdint.h>

/* ── SMP capability ───────────────────────────────────────────────────────
 * Core-affinity task creation and spinlock critical sections exist only on
 * genuinely multi-core kernels. Detect after the kernel headers so the port
 * config macros are visible.  */
#if defined(ESP_PLATFORM) || \
    (defined(portNUM_PROCESSORS) && (portNUM_PROCESSORS > 1)) || \
    (defined(configNUM_CORES) && (configNUM_CORES > 1))
#  define HF_OS_FREERTOS_SMP 1
#else
#  define HF_OS_FREERTOS_SMP 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Generic handle types used by the wrapper. */
typedef TaskHandle_t            OS_Thread;
typedef SemaphoreHandle_t       OS_Mutex;
typedef SemaphoreHandle_t       OS_Semaphore;
typedef QueueHandle_t           OS_Queue;
typedef EventGroupHandle_t      OS_EventGroup;
typedef TimerHandle_t           OS_Timer;
typedef StreamBufferHandle_t    OS_StreamBuffer;

/** Common integral aliases. */
typedef unsigned long           OS_Ulong;
typedef unsigned int            OS_Uint;

enum {
    OS_WAIT_FOREVER = portMAX_DELAY,
    OS_NO_WAIT      = 0,
    OS_SUCCESS      = 0,
    OS_AND          = 1,
    OS_OR           = 2,
    OS_AUTO_START   = 1,
    OS_DONT_START   = 0,
    OS_INHERIT      = 0
};

static inline OS_Ulong os_time_get(void) { return xTaskGetTickCount(); }

/* Thread wrappers ---------------------------------------------------------*/
typedef struct {
    void (*entry)(OS_Ulong);
    OS_Ulong arg;
} os_thread_start_t;

static inline void os_thread_start_trampoline(void *p)
{
    os_thread_start_t params = *(os_thread_start_t *)p;
    vPortFree(p);
    params.entry(params.arg);
}

static inline OS_Uint os_thread_create(OS_Thread *t, const char *name,
                                       void (*entry)(OS_Ulong),
                                       OS_Ulong input, uint8_t *stack,
                                       OS_Ulong stack_size, OS_Uint priority,
                                       OS_Uint preempt, OS_Ulong slice,
                                       OS_Uint auto_start)
{
    /* xTaskCreate allocates the stack. stack/preempt/slice stay in the
     * ThreadX-shaped signature; this backend does not install them. */
    (void)stack;
    (void)preempt;
    (void)slice;
    os_thread_start_t *params =
        (os_thread_start_t *)pvPortMalloc(sizeof(os_thread_start_t));
    if (!params) return 1;
    params->entry = entry;
    params->arg = input;
    BaseType_t res = xTaskCreate(os_thread_start_trampoline, name,
                                 stack_size / sizeof(StackType_t), params,
                                 priority, t);
    if (res != pdPASS) {
        vPortFree(params);
        return 1;
    }
    if (!auto_start)
        vTaskSuspend(*t);
    return OS_SUCCESS;
}

/**
 * @brief Core-affinity variant of @ref os_thread_create.
 *
 * @param core_id CPU core to pin to; negative means "no affinity".
 *
 * On a single-core kernel there is nothing to pin to, so this degrades to
 * @ref os_thread_create and @p core_id is ignored. That is a property of the
 * kernel, not a build workaround.
 */
static inline OS_Uint os_thread_create_pinned(OS_Thread *t, const char *name,
                                              void (*entry)(OS_Ulong),
                                              OS_Ulong input, uint8_t *stack,
                                              OS_Ulong stack_size, OS_Uint priority,
                                              OS_Uint preempt, OS_Ulong slice,
                                              OS_Uint auto_start, int core_id)
{
#if HF_OS_FREERTOS_SMP
    os_thread_start_t *params =
        (os_thread_start_t *)pvPortMalloc(sizeof(os_thread_start_t));
    if (!params) return 1;
    params->entry = entry;
    params->arg = input;
    const BaseType_t affinity =
        (core_id < 0) ? tskNO_AFFINITY : (BaseType_t)core_id;
    BaseType_t res = xTaskCreatePinnedToCore(os_thread_start_trampoline, name,
                                             stack_size / sizeof(StackType_t),
                                             params, priority, t, affinity);
    if (res != pdPASS) {
        vPortFree(params);
        return 1;
    }
    if (!auto_start)
        vTaskSuspend(*t);
    return OS_SUCCESS;
#else
    (void)core_id;
    return os_thread_create(t, name, entry, input, stack, stack_size,
                            priority, preempt, slice, auto_start);
#endif
}

/** Core index the calling task runs on; always 0 on single-core kernels. */
static inline int os_get_current_core_id(void)
{
#if HF_OS_FREERTOS_SMP
    return (int)xPortGetCoreID();
#else
    return 0;
#endif
}

static inline OS_Uint os_thread_resume(OS_Thread *t)   { vTaskResume(*t); return OS_SUCCESS; }
static inline OS_Uint os_thread_suspend(OS_Thread *t)  { vTaskSuspend(*t); return OS_SUCCESS; }
static inline OS_Uint os_thread_delete(OS_Thread *t)   { vTaskDelete(*t);  return OS_SUCCESS; }
static inline OS_Uint os_thread_terminate(OS_Thread *t){ vTaskDelete(*t);  return OS_SUCCESS; }
static inline OS_Uint os_thread_info_get(OS_Thread *t, OS_Uint *state)
{
    if (state)
        *state = eTaskGetState(*t);
    return OS_SUCCESS;
}
static inline OS_Uint os_thread_sleep(OS_Ulong ticks)  { vTaskDelay(ticks); return OS_SUCCESS; }
static inline OS_Uint os_thread_yield(void)
{ taskYIELD(); return OS_SUCCESS; }
static inline int os_kernel_is_running(void)
{ return xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED; }
static inline OS_Uint os_thread_is_current(const OS_Thread *t)
{ return (t != 0 && *t != 0 && xTaskGetCurrentTaskHandle() == *t) ? 1u : 0u; }

/** Portable thread-state aliases (FreeRTOS eTaskState). */
enum {
    OS_THREAD_READY      = eReady,
    OS_THREAD_SUSPENDED  = eSuspended,
    OS_THREAD_BLOCKED    = eBlocked
};

static inline int os_thread_is_live(const OS_Thread *t)
{
    return (t != 0) && (*t != 0);
}
static inline void os_thread_clear(OS_Thread *t)
{
    if (t) {
        *t = 0;
    }
}
static inline int os_mutex_is_live(const OS_Mutex *m)
{
    return (m != 0) && (*m != 0);
}
static inline int os_thread_state_is_suspended(OS_Uint state)
{
    return (state == (OS_Uint)eSuspended) || (state == (OS_Uint)eBlocked);
}

/* Mutex wrappers ---------------------------------------------------------*/
static inline OS_Uint os_mutex_create(OS_Mutex *m, const char *name, OS_Uint inherit)
{
    (void)name;
    (void)inherit;
    *m = xSemaphoreCreateMutex();
    return (*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_mutex_get(OS_Mutex *m, OS_Ulong wait)   { return xSemaphoreTake(*m, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_mutex_put(OS_Mutex *m)                  { return xSemaphoreGive(*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_mutex_delete(OS_Mutex *m)               { vSemaphoreDelete(*m); return OS_SUCCESS; }

/* Recursive Mutex wrappers -----------------------------------------------*/
static inline OS_Uint os_recursive_mutex_create(OS_Mutex *m, const char *name)
{
    (void)name;
    *m = xSemaphoreCreateRecursiveMutex();
    return (*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_recursive_mutex_get(OS_Mutex *m, OS_Ulong wait)   { return xSemaphoreTakeRecursive(*m, wait) == pdTRUE ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_recursive_mutex_put(OS_Mutex *m)                  { return xSemaphoreGiveRecursive(*m) == pdTRUE ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_recursive_mutex_delete(OS_Mutex *m)               { vSemaphoreDelete(*m); return OS_SUCCESS; }

/* Semaphore wrappers -----------------------------------------------------*/
static inline OS_Uint os_semaphore_create(OS_Semaphore *s, const char* name, OS_Uint initial)
{
    (void)name;
    *s = xSemaphoreCreateCounting(UINT16_MAX, initial);
    return (*s) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_semaphore_delete(OS_Semaphore *s)   { vSemaphoreDelete(*s); return OS_SUCCESS; }
static inline OS_Uint os_semaphore_put(OS_Semaphore *s)      { return xSemaphoreGive(*s) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_semaphore_get(OS_Semaphore *s, OS_Ulong wait) { return xSemaphoreTake(*s, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_semaphore_info_get(OS_Semaphore *s, OS_Ulong *count)
{
    if (count)
        *count = uxSemaphoreGetCount(*s);
    return OS_SUCCESS;
}

/* Queue wrappers ---------------------------------------------------------*/
static inline OS_Uint os_queue_create(OS_Queue *q, const char* name, OS_Uint item_words,
                                      void* store, OS_Ulong length)
{
    (void)name;
    (void)store;
    *q = xQueueCreate(length, item_words * sizeof(uint32_t));
    return (*q) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_queue_delete(OS_Queue *q)    { vQueueDelete(*q); return OS_SUCCESS; }
static inline OS_Uint os_queue_send(OS_Queue *q, void *msg, OS_Ulong wait)    { return xQueueSend(*q, msg, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_queue_receive(OS_Queue *q, void *msg, OS_Ulong wait) { return xQueueReceive(*q, msg, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }

/* Event group wrappers ---------------------------------------------------*/
static inline OS_Uint os_event_group_create(OS_EventGroup *g, const char* name)
{
    (void)name;
    *g = xEventGroupCreate();
    return (*g) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_event_group_delete(OS_EventGroup *g) { vEventGroupDelete(*g); return OS_SUCCESS; }
static inline OS_Uint os_event_group_set(OS_EventGroup *g, OS_Ulong flags)
{ xEventGroupSetBits(*g, flags); return OS_SUCCESS; }
static inline OS_Uint os_event_group_clear(OS_EventGroup *g, OS_Ulong flags)
{ xEventGroupClearBits(*g, flags); return OS_SUCCESS; }
static inline OS_Uint os_event_group_get(OS_EventGroup *g, OS_Ulong flags, OS_Uint option,
                                         OS_Ulong *actual, OS_Ulong wait)
{
    EventBits_t bits = xEventGroupWaitBits(*g, flags, option == OS_AND,
                                           option == OS_AND, wait);
    if (actual)
        *actual = bits;
    return OS_SUCCESS;
}

/* Timer wrappers ---------------------------------------------------------*/
typedef struct {
    void (*cb)(OS_Ulong);
    OS_Ulong id;
} os_timer_cb_t;

static inline void os_timer_trampoline(TimerHandle_t timer)
{
    os_timer_cb_t *params = (os_timer_cb_t *)pvTimerGetTimerID(timer);
    params->cb(params->id);
}

static inline OS_Uint os_timer_create(OS_Timer *t, const char *name,
                                      void (*cb)(OS_Ulong), OS_Ulong id,
                                      OS_Ulong initial, OS_Ulong reload,
                                      OS_Uint auto_act)
{
    os_timer_cb_t *params = (os_timer_cb_t *)pvPortMalloc(sizeof(os_timer_cb_t));
    if (!params) return 1;
    params->cb = cb;
    params->id = id;

    *t = xTimerCreate(name, initial, reload != 0, (void *)params,
                      os_timer_trampoline);
    if (!*t) {
        vPortFree(params);
        return 1;
    }
    if (auto_act)
        xTimerStart(*t, 0);
    return OS_SUCCESS;
}
static inline OS_Uint os_timer_delete(OS_Timer *t)
{
    os_timer_cb_t *params = (os_timer_cb_t *)pvTimerGetTimerID(*t);
    vPortFree(params);
    xTimerDelete(*t, 0);
    return OS_SUCCESS;
}
static inline OS_Uint os_timer_activate(OS_Timer *t)   { return xTimerStart(*t,0)==pdPASS ? OS_SUCCESS:(OS_Uint)1; }
static inline OS_Uint os_timer_deactivate(OS_Timer *t) { return xTimerStop(*t,0)==pdPASS ? OS_SUCCESS:(OS_Uint)1; }

/* Stream buffer wrappers ------------------------------------------------*/
static inline OS_Uint os_stream_buffer_create(OS_StreamBuffer *b, size_t capacity, size_t trigger)
{
    *b = xStreamBufferCreate(capacity, trigger);
    return (*b) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_stream_buffer_delete(OS_StreamBuffer *b) { vStreamBufferDelete(*b); return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_send(OS_StreamBuffer *b, const void *data, size_t len, OS_Ulong wait)
{ return xStreamBufferSend(*b, data, len, wait) == len ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_stream_buffer_receive(OS_StreamBuffer *b, void *data, size_t len, OS_Ulong wait)
{ return xStreamBufferReceive(*b, data, len, wait) > 0 ? OS_SUCCESS : (OS_Uint)1; }

/* Critical section wrappers ----------------------------------------------
 * SMP kernels need an explicit spinlock; single-core kernels get mutual
 * exclusion from the kernel's own ISR-masking critical section. The storage
 * is declared in both flavors so CriticalSection.cpp stays flavor-neutral. */
#if HF_OS_FREERTOS_SMP
typedef portMUX_TYPE OS_Critical;
extern OS_Critical os_critical_mux;
static inline void os_critical_enter(void)  { portENTER_CRITICAL(&os_critical_mux); }
static inline void os_critical_exit(void)   { portEXIT_CRITICAL(&os_critical_mux); }
#else
/** Unused on single-core; kept so the storage symbol is flavor-independent. */
typedef uint8_t OS_Critical;
extern OS_Critical os_critical_mux;
static inline void os_critical_enter(void)  { portENTER_CRITICAL(); }
static inline void os_critical_exit(void)   { portEXIT_CRITICAL(); }
#endif

#ifdef __cplusplus
}
#endif
