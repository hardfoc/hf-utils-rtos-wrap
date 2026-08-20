#pragma once
/**
 * @file OsAbstraction.h
 * @brief Minimal generic RTOS abstraction used by the component.
 *
 * This header wraps the underlying RTOS (FreeRTOS on ESP-IDF) with a set of
 * generic types and helper functions.  None of the exported names expose the
 * specific RTOS being used so that a different implementation can be provided
 * in the future.
 */

/* ── RTOS selection guard ────────────────────────────────────────────────── */
#if defined(HF_RTOS_FREERTOS)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/stream_buffer.h"

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
                                       OS_Uint /*preempt*/, OS_Ulong /*slice*/,
                                       OS_Uint auto_start)
{
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
 * @brief Pinned-to-core variant of os_thread_create (FreeRTOS / SMP).
 *
 * @param core_id  CPU core to pin the task to. Pass `tskNO_AFFINITY`
 *                 (or any value < 0 / >= portNUM_PROCESSORS) for no
 *                 affinity (equivalent to `xTaskCreate`).
 *
 * On single-core builds (portNUM_PROCESSORS == 1) the core_id is
 * ignored — `xTaskCreatePinnedToCore` is still safe but has no effect.
 */
static inline OS_Uint os_thread_create_pinned(OS_Thread *t, const char *name,
                                              void (*entry)(OS_Ulong),
                                              OS_Ulong input, uint8_t *stack,
                                              OS_Ulong stack_size, OS_Uint priority,
                                              OS_Uint /*preempt*/, OS_Ulong /*slice*/,
                                              OS_Uint auto_start, int core_id)
{
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
}

/// Returns the FreeRTOS core ID the calling task is currently running on
/// (0 or 1 on ESP32-S3). Returns 0 on single-core builds.
static inline int os_get_current_core_id(void)
{
#if (configNUM_CORES > 1) || (portNUM_PROCESSORS > 1)
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
static inline OS_Uint os_mutex_create(OS_Mutex *m, const char * /*name*/, OS_Uint /*inherit*/)
{
    *m = xSemaphoreCreateMutex();
    return (*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_mutex_get(OS_Mutex *m, OS_Ulong wait)   { return xSemaphoreTake(*m, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_mutex_put(OS_Mutex *m)                  { return xSemaphoreGive(*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_mutex_delete(OS_Mutex *m)               { vSemaphoreDelete(*m); return OS_SUCCESS; }

/* Recursive Mutex wrappers -----------------------------------------------*/
static inline OS_Uint os_recursive_mutex_create(OS_Mutex *m, const char * /*name*/)
{
    *m = xSemaphoreCreateRecursiveMutex();
    return (*m) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_recursive_mutex_get(OS_Mutex *m, OS_Ulong wait)   { return xSemaphoreTakeRecursive(*m, wait) == pdTRUE ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_recursive_mutex_put(OS_Mutex *m)                  { return xSemaphoreGiveRecursive(*m) == pdTRUE ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_recursive_mutex_delete(OS_Mutex *m)               { vSemaphoreDelete(*m); return OS_SUCCESS; }

/* Semaphore wrappers -----------------------------------------------------*/
static inline OS_Uint os_semaphore_create(OS_Semaphore *s, const char* /*name*/, OS_Uint initial)
{
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
static inline OS_Uint os_queue_create(OS_Queue *q, const char* /*name*/, OS_Uint item_words,
                                      void* /*store*/, OS_Ulong length)
{
    *q = xQueueCreate(length, item_words * sizeof(uint32_t));
    return (*q) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_queue_delete(OS_Queue *q)    { vQueueDelete(*q); return OS_SUCCESS; }
static inline OS_Uint os_queue_send(OS_Queue *q, void *msg, OS_Ulong wait)    { return xQueueSend(*q, msg, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }
static inline OS_Uint os_queue_receive(OS_Queue *q, void *msg, OS_Ulong wait) { return xQueueReceive(*q, msg, wait) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1; }

/* Event group wrappers ---------------------------------------------------*/
static inline OS_Uint os_event_group_create(OS_EventGroup *g, const char* /*name*/)
{
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

/* Critical section wrappers ----------------------------------------------*/
typedef portMUX_TYPE OS_Critical;
extern OS_Critical os_critical_mux;
static inline void os_critical_enter(void)  { portENTER_CRITICAL(&os_critical_mux); }
static inline void os_critical_exit(void)   { portEXIT_CRITICAL(&os_critical_mux); }

#ifdef __cplusplus
}
#endif

#elif defined(HF_RTOS_THREADX)

#include <stdint.h>
#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef TX_THREAD              OS_Thread;
typedef TX_MUTEX               OS_Mutex;
typedef TX_SEMAPHORE           OS_Semaphore;
typedef TX_QUEUE               OS_Queue;
typedef TX_EVENT_FLAGS_GROUP   OS_EventGroup;
typedef TX_TIMER               OS_Timer;
typedef void*                  OS_StreamBuffer;

typedef unsigned long          OS_Ulong;
typedef unsigned int           OS_Uint;

enum {
    OS_WAIT_FOREVER = (int)TX_WAIT_FOREVER,
    OS_NO_WAIT      = (int)TX_NO_WAIT,
    OS_SUCCESS      = (int)TX_SUCCESS,
    OS_AND          = (int)TX_AND,
    OS_OR           = (int)TX_OR,
    OS_AUTO_START   = (int)TX_AUTO_START,
    OS_DONT_START   = (int)TX_DONT_START,
    OS_INHERIT      = (int)TX_INHERIT
};

static inline OS_Ulong os_time_get(void) { return (OS_Ulong)tx_time_get(); }

static inline OS_Uint os_thread_create(OS_Thread *t, const char *name,
                                       void (*entry)(OS_Ulong),
                                       OS_Ulong input, uint8_t *stack,
                                       OS_Ulong stack_size, OS_Uint priority,
                                       OS_Uint preempt, OS_Ulong slice,
                                       OS_Uint auto_start)
{
    const UINT prio = (priority == 0u) ? 16u : (UINT)priority;
    const UINT pre = (preempt == 0u) ? prio : (UINT)preempt;
    const ULONG ts = (slice == 0u) ? TX_NO_TIME_SLICE : (ULONG)slice;
    const UINT as = auto_start ? TX_AUTO_START : TX_DONT_START;
    return (OS_Uint)tx_thread_create(t, (CHAR*)name, (VOID (*)(ULONG))entry,
                                     (ULONG)input, stack, (ULONG)stack_size,
                                     prio, pre, ts, as);
}

static inline OS_Uint os_thread_create_pinned(OS_Thread *t, const char *name,
                                              void (*entry)(OS_Ulong),
                                              OS_Ulong input, uint8_t *stack,
                                              OS_Ulong stack_size, OS_Uint priority,
                                              OS_Uint preempt, OS_Ulong slice,
                                              OS_Uint auto_start, int core_id)
{
    (void)core_id;
    return os_thread_create(t, name, entry, input, stack, stack_size,
                            priority, preempt, slice, auto_start);
}

static inline int os_get_current_core_id(void) { return 0; }
static inline OS_Uint os_thread_resume(OS_Thread *t)
{ return (OS_Uint)tx_thread_resume(t); }
static inline OS_Uint os_thread_suspend(OS_Thread *t)
{ return (OS_Uint)tx_thread_suspend(t); }
static inline OS_Uint os_thread_delete(OS_Thread *t)
{ return (OS_Uint)tx_thread_delete(t); }
static inline OS_Uint os_thread_terminate(OS_Thread *t)
{ return (OS_Uint)tx_thread_terminate(t); }
static inline OS_Uint os_thread_info_get(OS_Thread *t, OS_Uint *state)
{
    /* ThreadX TX_NULL is (void*)0 and is not a typed C++ null. */
    CHAR *unused_name = 0;
    ULONG unused_run = 0;
    UINT unused_prio = 0;
    UINT unused_preempt = 0;
    ULONG unused_slice = 0;
    TX_THREAD *unused_next = 0;
    TX_THREAD *unused_suspended = 0;
    UINT s = 0;
    const UINT err = tx_thread_info_get(t, &unused_name, &s, &unused_run,
                                        &unused_prio, &unused_preempt,
                                        &unused_slice, &unused_next,
                                        &unused_suspended);
    if (state) {
        *state = (OS_Uint)s;
    }
    return (OS_Uint)err;
}
static inline OS_Uint os_thread_sleep(OS_Ulong ticks)
{ return (OS_Uint)tx_thread_sleep((ULONG)ticks); }

static inline OS_Uint os_mutex_create(OS_Mutex *m, const char *name, OS_Uint inherit)
{
    return (OS_Uint)tx_mutex_create(m, (CHAR*)name,
                                    inherit ? TX_INHERIT : TX_NO_INHERIT);
}
static inline OS_Uint os_mutex_get(OS_Mutex *m, OS_Ulong wait)
{ return (OS_Uint)tx_mutex_get(m, (ULONG)wait); }
static inline OS_Uint os_mutex_put(OS_Mutex *m)
{ return (OS_Uint)tx_mutex_put(m); }
static inline OS_Uint os_mutex_delete(OS_Mutex *m)
{ return (OS_Uint)tx_mutex_delete(m); }

static inline OS_Uint os_recursive_mutex_create(OS_Mutex *m, const char *name)
{ return os_mutex_create(m, name, OS_INHERIT); }
static inline OS_Uint os_recursive_mutex_get(OS_Mutex *m, OS_Ulong wait)
{ return os_mutex_get(m, wait); }
static inline OS_Uint os_recursive_mutex_put(OS_Mutex *m)
{ return os_mutex_put(m); }
static inline OS_Uint os_recursive_mutex_delete(OS_Mutex *m)
{ return os_mutex_delete(m); }

static inline OS_Uint os_semaphore_create(OS_Semaphore *s, const char *name, OS_Uint initial)
{
    return (OS_Uint)tx_semaphore_create(s, (CHAR*)name, (ULONG)initial);
}
static inline OS_Uint os_semaphore_delete(OS_Semaphore *s)
{ return (OS_Uint)tx_semaphore_delete(s); }
static inline OS_Uint os_semaphore_put(OS_Semaphore *s)
{ return (OS_Uint)tx_semaphore_put(s); }
static inline OS_Uint os_semaphore_get(OS_Semaphore *s, OS_Ulong wait)
{ return (OS_Uint)tx_semaphore_get(s, (ULONG)wait); }
static inline OS_Uint os_semaphore_info_get(OS_Semaphore *s, OS_Ulong *count)
{
    CHAR *unused_name = 0;
    TX_THREAD *unused_first = 0;
    ULONG unused_suspended = 0;
    TX_SEMAPHORE *unused_next = 0;
    ULONG c = 0;
    const UINT err = tx_semaphore_info_get(s, &unused_name, &c, &unused_first,
                                           &unused_suspended, &unused_next);
    if (count) {
        *count = (OS_Ulong)c;
    }
    return (OS_Uint)err;
}

/** Portable thread-state aliases (ThreadX tx_thread_state). */
enum {
    OS_THREAD_READY      = TX_READY,
    OS_THREAD_SUSPENDED  = TX_SUSPENDED,
    OS_THREAD_BLOCKED    = TX_SLEEP
};

static inline int os_thread_is_live(const OS_Thread *t)
{
    return (t != 0) && (t->tx_thread_id != 0u);
}
static inline void os_thread_clear(OS_Thread *t)
{
    if (t) {
        t->tx_thread_id = 0u;
    }
}
static inline int os_mutex_is_live(const OS_Mutex *m)
{
    return (m != 0) && (m->tx_mutex_id != 0u);
}
static inline int os_thread_state_is_suspended(OS_Uint state)
{
    return (state != (OS_Uint)TX_READY) &&
           (state != (OS_Uint)TX_COMPLETED) &&
           (state != (OS_Uint)TX_TERMINATED);
}

static inline OS_Uint os_queue_create(OS_Queue *q, const char *name, OS_Uint item_words,
                                      void *store, OS_Ulong length)
{
    return (OS_Uint)tx_queue_create(q, (CHAR*)name, (UINT)item_words, store,
                                    (ULONG)(length * item_words * sizeof(ULONG)));
}
static inline OS_Uint os_queue_delete(OS_Queue *q)
{ return (OS_Uint)tx_queue_delete(q); }
static inline OS_Uint os_queue_send(OS_Queue *q, void *msg, OS_Ulong wait)
{ return (OS_Uint)tx_queue_send(q, msg, (ULONG)wait); }
static inline OS_Uint os_queue_receive(OS_Queue *q, void *msg, OS_Ulong wait)
{ return (OS_Uint)tx_queue_receive(q, msg, (ULONG)wait); }

static inline OS_Uint os_event_group_create(OS_EventGroup *g, const char *name)
{ return (OS_Uint)tx_event_flags_create(g, (CHAR*)name); }
static inline OS_Uint os_event_group_delete(OS_EventGroup *g)
{ return (OS_Uint)tx_event_flags_delete(g); }
static inline OS_Uint os_event_group_set(OS_EventGroup *g, OS_Ulong flags)
{ return (OS_Uint)tx_event_flags_set(g, (ULONG)flags, TX_OR); }
static inline OS_Uint os_event_group_clear(OS_EventGroup *g, OS_Ulong flags)
{ return (OS_Uint)tx_event_flags_set(g, (ULONG)~flags, TX_AND); }
static inline OS_Uint os_event_group_get(OS_EventGroup *g, OS_Ulong flags, OS_Uint option,
                                         OS_Ulong *actual, OS_Ulong wait)
{
    ULONG got = 0;
    const UINT opt = (option == OS_AND) ? TX_AND : TX_OR;
    const UINT err = tx_event_flags_get(g, (ULONG)flags, opt, &got, (ULONG)wait);
    if (actual) {
        *actual = (OS_Ulong)got;
    }
    return (OS_Uint)err;
}

static inline OS_Uint os_timer_create(OS_Timer *t, const char *name,
                                      void (*cb)(OS_Ulong), OS_Ulong id,
                                      OS_Ulong initial, OS_Ulong reload,
                                      OS_Uint auto_act)
{
    const UINT err = tx_timer_create(t, (CHAR*)name, (VOID (*)(ULONG))cb,
                                     (ULONG)id, (ULONG)initial,
                                     (ULONG)reload, TX_NO_ACTIVATE);
    if (err != TX_SUCCESS) {
        return (OS_Uint)err;
    }
    if (auto_act) {
        return (OS_Uint)tx_timer_activate(t);
    }
    return OS_SUCCESS;
}
static inline OS_Uint os_timer_delete(OS_Timer *t)
{ return (OS_Uint)tx_timer_delete(t); }
static inline OS_Uint os_timer_activate(OS_Timer *t)
{ return (OS_Uint)tx_timer_activate(t); }
static inline OS_Uint os_timer_deactivate(OS_Timer *t)
{ return (OS_Uint)tx_timer_deactivate(t); }

static inline OS_Uint os_stream_buffer_create(OS_StreamBuffer *b, size_t capacity, size_t trigger)
{ (void)capacity; (void)trigger; *b = 0; return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_delete(OS_StreamBuffer *b)
{ *b = 0; return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_send(OS_StreamBuffer *b, const void *data, size_t len, OS_Ulong wait)
{ (void)b; (void)data; (void)len; (void)wait; return (OS_Uint)1; }
static inline OS_Uint os_stream_buffer_receive(OS_StreamBuffer *b, void *data, size_t len, OS_Ulong wait)
{ (void)b; (void)data; (void)len; (void)wait; return (OS_Uint)1; }

typedef UINT OS_Critical;
extern OS_Critical os_critical_mux;
static inline void os_critical_enter(void)
{ os_critical_mux = tx_interrupt_control(TX_INT_DISABLE); }
static inline void os_critical_exit(void)
{ (void)tx_interrupt_control(os_critical_mux); }

#ifdef __cplusplus
}
#endif

#else /* HF_RTOS_NONE (or no RTOS selected) — bare-metal / host-only stubs */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Stub handle types (opaque pointers, never dereferenced) ──────────── */
typedef void*           OS_Thread;
typedef void*           OS_Mutex;
typedef void*           OS_Semaphore;
typedef void*           OS_Queue;
typedef void*           OS_EventGroup;
typedef void*           OS_Timer;
typedef void*           OS_StreamBuffer;

typedef unsigned long   OS_Ulong;
typedef unsigned int    OS_Uint;

enum {
    OS_WAIT_FOREVER = 0xFFFFFFFFUL,
    OS_NO_WAIT      = 0,
    OS_SUCCESS      = 0,
    OS_AND          = 1,
    OS_OR           = 2,
    OS_AUTO_START   = 1,
    OS_DONT_START   = 0,
    OS_INHERIT      = 0
};

static inline OS_Ulong os_time_get(void) { return 0; }

/* Thread — no-ops (single-threaded) */
typedef struct { void (*entry)(OS_Ulong); OS_Ulong arg; } os_thread_start_t;
static inline OS_Uint os_thread_create(OS_Thread *t, const char *name,
                                       void (*entry)(OS_Ulong),
                                       OS_Ulong input, uint8_t *stack,
                                       OS_Ulong stack_size, OS_Uint priority,
                                       OS_Uint preempt, OS_Ulong slice,
                                       OS_Uint auto_start)
{ (void)t; (void)name; (void)entry; (void)input; (void)stack;
  (void)stack_size; (void)priority; (void)preempt; (void)slice;
  (void)auto_start; return OS_SUCCESS; }
/* Bare-metal stub — core_id is ignored. */
static inline OS_Uint os_thread_create_pinned(OS_Thread *t, const char *name,
                                              void (*entry)(OS_Ulong),
                                              OS_Ulong input, uint8_t *stack,
                                              OS_Ulong stack_size, OS_Uint priority,
                                              OS_Uint preempt, OS_Ulong slice,
                                              OS_Uint auto_start, int core_id)
{ (void)core_id;
  return os_thread_create(t, name, entry, input, stack, stack_size,
                          priority, preempt, slice, auto_start); }
static inline int os_get_current_core_id(void) { return 0; }
static inline OS_Uint os_thread_resume(OS_Thread *t)    { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_thread_suspend(OS_Thread *t)   { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_thread_delete(OS_Thread *t)    { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_thread_terminate(OS_Thread *t) { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_thread_info_get(OS_Thread *t, OS_Uint *state)
{ (void)t; if (state) *state = 0; return OS_SUCCESS; }
static inline OS_Uint os_thread_sleep(OS_Ulong ticks)   { (void)ticks; return OS_SUCCESS; }

enum {
    OS_THREAD_READY      = 0,
    OS_THREAD_SUSPENDED  = 1,
    OS_THREAD_BLOCKED    = 2
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
    (void)state;
    return 0;
}

/* Mutex — always succeeds (single-threaded, no contention) */
static inline OS_Uint os_mutex_create(OS_Mutex *m, const char *name, OS_Uint inherit)
{ (void)m; (void)name; (void)inherit; return OS_SUCCESS; }
static inline OS_Uint os_mutex_get(OS_Mutex *m, OS_Ulong wait)   { (void)m; (void)wait; return OS_SUCCESS; }
static inline OS_Uint os_mutex_put(OS_Mutex *m)                  { (void)m; return OS_SUCCESS; }
static inline OS_Uint os_mutex_delete(OS_Mutex *m)               { (void)m; return OS_SUCCESS; }

/* Recursive Mutex — always succeeds (single-threaded, no contention) */
static inline OS_Uint os_recursive_mutex_create(OS_Mutex *m, const char *name)
{ (void)m; (void)name; return OS_SUCCESS; }
static inline OS_Uint os_recursive_mutex_get(OS_Mutex *m, OS_Ulong wait)   { (void)m; (void)wait; return OS_SUCCESS; }
static inline OS_Uint os_recursive_mutex_put(OS_Mutex *m)                  { (void)m; return OS_SUCCESS; }
static inline OS_Uint os_recursive_mutex_delete(OS_Mutex *m)               { (void)m; return OS_SUCCESS; }

/* Semaphore — always succeeds */
static inline OS_Uint os_semaphore_create(OS_Semaphore *s, const char *name, OS_Uint initial)
{ (void)s; (void)name; (void)initial; return OS_SUCCESS; }
static inline OS_Uint os_semaphore_delete(OS_Semaphore *s)   { (void)s; return OS_SUCCESS; }
static inline OS_Uint os_semaphore_put(OS_Semaphore *s)      { (void)s; return OS_SUCCESS; }
static inline OS_Uint os_semaphore_get(OS_Semaphore *s, OS_Ulong wait) { (void)s; (void)wait; return OS_SUCCESS; }
static inline OS_Uint os_semaphore_info_get(OS_Semaphore *s, OS_Ulong *count)
{ (void)s; if (count) *count = 0; return OS_SUCCESS; }

/* Queue — always succeeds (data is silently discarded / zeroed) */
static inline OS_Uint os_queue_create(OS_Queue *q, const char *name, OS_Uint item_words,
                                      void *store, OS_Ulong length)
{ (void)q; (void)name; (void)item_words; (void)store; (void)length; return OS_SUCCESS; }
static inline OS_Uint os_queue_delete(OS_Queue *q)    { (void)q; return OS_SUCCESS; }
static inline OS_Uint os_queue_send(OS_Queue *q, void *msg, OS_Ulong wait)
{ (void)q; (void)msg; (void)wait; return OS_SUCCESS; }
static inline OS_Uint os_queue_receive(OS_Queue *q, void *msg, OS_Ulong wait)
{ (void)q; (void)msg; (void)wait; return (OS_Uint)1; /* nothing to receive */ }

/* Event group — always succeeds */
static inline OS_Uint os_event_group_create(OS_EventGroup *g, const char *name)
{ (void)g; (void)name; return OS_SUCCESS; }
static inline OS_Uint os_event_group_delete(OS_EventGroup *g) { (void)g; return OS_SUCCESS; }
static inline OS_Uint os_event_group_set(OS_EventGroup *g, OS_Ulong flags)
{ (void)g; (void)flags; return OS_SUCCESS; }
static inline OS_Uint os_event_group_clear(OS_EventGroup *g, OS_Ulong flags)
{ (void)g; (void)flags; return OS_SUCCESS; }
static inline OS_Uint os_event_group_get(OS_EventGroup *g, OS_Ulong flags, OS_Uint option,
                                         OS_Ulong *actual, OS_Ulong wait)
{ (void)g; (void)flags; (void)option; (void)wait; if (actual) *actual = 0; return OS_SUCCESS; }

/* Timer — no-ops (no scheduler) */
typedef struct { void (*cb)(OS_Ulong); OS_Ulong id; } os_timer_cb_t;
static inline OS_Uint os_timer_create(OS_Timer *t, const char *name,
                                      void (*cb)(OS_Ulong), OS_Ulong id,
                                      OS_Ulong initial, OS_Ulong reload,
                                      OS_Uint auto_act)
{ (void)t; (void)name; (void)cb; (void)id; (void)initial; (void)reload;
  (void)auto_act; return OS_SUCCESS; }
static inline OS_Uint os_timer_delete(OS_Timer *t)     { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_timer_activate(OS_Timer *t)   { (void)t; return OS_SUCCESS; }
static inline OS_Uint os_timer_deactivate(OS_Timer *t) { (void)t; return OS_SUCCESS; }

/* Stream buffer — no-ops */
static inline OS_Uint os_stream_buffer_create(OS_StreamBuffer *b, size_t capacity, size_t trigger)
{ (void)b; (void)capacity; (void)trigger; return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_delete(OS_StreamBuffer *b) { (void)b; return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_send(OS_StreamBuffer *b, const void *data, size_t len, OS_Ulong wait)
{ (void)b; (void)data; (void)len; (void)wait; return OS_SUCCESS; }
static inline OS_Uint os_stream_buffer_receive(OS_StreamBuffer *b, void *data, size_t len, OS_Ulong wait)
{ (void)b; (void)data; (void)len; (void)wait; return (OS_Uint)1; }

/* Critical section — no-ops (single-threaded) */
typedef int OS_Critical;
static inline void os_critical_enter(void) {}
static inline void os_critical_exit(void)  {}

#ifdef __cplusplus
}
#endif

#endif /* HF_RTOS_FREERTOS / HF_RTOS_NONE */

