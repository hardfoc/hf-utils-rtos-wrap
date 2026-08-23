#pragma once
/**
 * @file os_cmsis.h
 * @brief CMSIS-RTOS2 backend for the generic OS abstraction.
 *
 * Included only by @ref OsAbstraction.h when `HF_RTOS_CMSIS` is selected, or
 * by the product BaseThread TU when `PW_RTOS_BACKEND=cmsis_rtos2`. Do not
 * include this header from apps.
 *
 * Create/sleep/yield are `static inline` 1:1 maps onto `osThreadNew` /
 * `osDelay` / `osThreadYield`. No extra delay and no heap trampoline on
 * @ref os_thread_spawn (argument is the caller's `void*`).
 *
 * @see os/README.md
 */

#include "cmsis_os2.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef osThreadId_t           OS_Thread;
typedef osMutexId_t            OS_Mutex;
typedef osSemaphoreId_t        OS_Semaphore;
typedef osMessageQueueId_t     OS_Queue;
typedef osEventFlagsId_t       OS_EventGroup;
typedef osTimerId_t            OS_Timer;
typedef void*                  OS_StreamBuffer;

typedef unsigned long          OS_Ulong;
typedef unsigned int           OS_Uint;

enum {
    OS_WAIT_FOREVER = (int)osWaitForever,
    OS_NO_WAIT      = 0,
    OS_SUCCESS      = 0,
    OS_AND          = 1,
    OS_OR           = 2,
    OS_AUTO_START   = 1,
    OS_DONT_START   = 0,
    OS_INHERIT      = 0
};

static inline OS_Ulong os_time_get(void)
{
    return (OS_Ulong)osKernelGetTickCount();
}

static inline int os_kernel_is_running(void)
{
    return osKernelGetState() == osKernelRunning;
}

static inline OS_Uint os_thread_yield(void)
{
    return (osThreadYield() == osOK) ? (OS_Uint)OS_SUCCESS : (OS_Uint)1;
}

static inline OS_Uint os_thread_is_current(const OS_Thread *t)
{
    return (t != 0 && *t != 0 && osThreadGetId() == *t) ? 1u : 0u;
}

/**
 * @brief Create a CMSIS thread. @p entry receives @p arg — no heap cookie.
 * @param stack optional static stack; null lets the kernel allocate.
 * @param stack_size stack bytes (CMSIS `osThreadAttr_t.stack_size`).
 */
static inline OS_Uint os_thread_spawn(OS_Thread *t, const char *name,
                                      void (*entry)(void *), void *arg,
                                      void *stack, OS_Ulong stack_size,
                                      OS_Uint priority, OS_Uint auto_start)
{
    if (t == 0 || entry == 0) {
        return (OS_Uint)1;
    }
    osThreadAttr_t attr = {0};
    attr.name = name;
    attr.stack_size = (uint32_t)stack_size;
    attr.stack_mem = stack;
    if (priority != 0u) {
        attr.priority = (osPriority_t)priority;
    }
    osThreadId_t id = osThreadNew(entry, arg, &attr);
    if (id == 0) {
        return (OS_Uint)1;
    }
    *t = id;
    if (!auto_start) {
        (void)osThreadSuspend(id);
    }
    return OS_SUCCESS;
}

typedef struct {
    void (*entry)(OS_Ulong);
    OS_Ulong arg;
} os_cmsis_start_t;

static inline void os_cmsis_generic_entry(void *p)
{
    os_cmsis_start_t params = *(os_cmsis_start_t *)p;
    free(p);
    params.entry(params.arg);
}

/** Generic create (hf-core BaseThread shape). Allocates a start cookie once. */
static inline OS_Uint os_thread_create(OS_Thread *t, const char *name,
                                       void (*entry)(OS_Ulong),
                                       OS_Ulong input, uint8_t *stack,
                                       OS_Ulong stack_size, OS_Uint priority,
                                       OS_Uint /*preempt*/, OS_Ulong /*slice*/,
                                       OS_Uint auto_start)
{
    os_cmsis_start_t *params = (os_cmsis_start_t *)malloc(sizeof(os_cmsis_start_t));
    if (params == 0) {
        return (OS_Uint)1;
    }
    params->entry = entry;
    params->arg = input;
    const OS_Uint err = os_thread_spawn(t, name, os_cmsis_generic_entry, params,
                                        stack, stack_size, priority, auto_start);
    if (err != OS_SUCCESS) {
        free(params);
    }
    return err;
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
{
    return (t && osThreadResume(*t) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_thread_suspend(OS_Thread *t)
{
    return (t && osThreadSuspend(*t) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_thread_delete(OS_Thread *t)
{
    return (t && osThreadTerminate(*t) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_thread_terminate(OS_Thread *t)
{
    return os_thread_delete(t);
}
static inline OS_Uint os_thread_info_get(OS_Thread *t, OS_Uint *state)
{
    if (state && t && *t) {
        *state = (OS_Uint)osThreadGetState(*t);
    }
    return OS_SUCCESS;
}
static inline OS_Uint os_thread_sleep(OS_Ulong ticks)
{
    return (osDelay((uint32_t)ticks) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}

enum {
    OS_THREAD_READY      = osThreadReady,
    OS_THREAD_SUSPENDED  = osThreadBlocked,
    OS_THREAD_BLOCKED    = osThreadBlocked
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
    return (state == (OS_Uint)osThreadBlocked) ||
           (state == (OS_Uint)osThreadInactive);
}

static inline OS_Uint os_mutex_create(OS_Mutex *m, const char *name, OS_Uint inherit)
{
    osMutexAttr_t attr = {0};
    attr.name = name;
    if (inherit) {
        attr.attr_bits = osMutexPrioInherit;
    }
    *m = osMutexNew(&attr);
    return (*m) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_mutex_get(OS_Mutex *m, OS_Ulong wait)
{
    return (osMutexAcquire(*m, (uint32_t)wait) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_mutex_put(OS_Mutex *m)
{
    return (osMutexRelease(*m) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_mutex_delete(OS_Mutex *m)
{
    return (osMutexDelete(*m) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}

static inline OS_Uint os_recursive_mutex_create(OS_Mutex *m, const char *name)
{
    osMutexAttr_t attr = {0};
    attr.name = name;
    attr.attr_bits = osMutexRecursive | osMutexPrioInherit;
    *m = osMutexNew(&attr);
    return (*m) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_recursive_mutex_get(OS_Mutex *m, OS_Ulong wait)
{
    return os_mutex_get(m, wait);
}
static inline OS_Uint os_recursive_mutex_put(OS_Mutex *m)
{
    return os_mutex_put(m);
}
static inline OS_Uint os_recursive_mutex_delete(OS_Mutex *m)
{
    return os_mutex_delete(m);
}

static inline OS_Uint os_semaphore_create(OS_Semaphore *s, const char *name, OS_Uint initial)
{
    osSemaphoreAttr_t attr = {0};
    attr.name = name;
    *s = osSemaphoreNew(0xFFFFu, initial, &attr);
    return (*s) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_semaphore_delete(OS_Semaphore *s)
{
    return (osSemaphoreDelete(*s) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_semaphore_put(OS_Semaphore *s)
{
    return (osSemaphoreRelease(*s) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_semaphore_get(OS_Semaphore *s, OS_Ulong wait)
{
    return (osSemaphoreAcquire(*s, (uint32_t)wait) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_semaphore_info_get(OS_Semaphore *s, OS_Ulong *count)
{
    if (count) {
        *count = (OS_Ulong)osSemaphoreGetCount(*s);
    }
    return OS_SUCCESS;
}

static inline OS_Uint os_queue_create(OS_Queue *q, const char *name, OS_Uint item_words,
                                      void * /*store*/, OS_Ulong length)
{
    osMessageQueueAttr_t attr = {0};
    attr.name = name;
    *q = osMessageQueueNew((uint32_t)length, item_words * sizeof(uint32_t), &attr);
    return (*q) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_queue_delete(OS_Queue *q)
{
    return (osMessageQueueDelete(*q) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_queue_send(OS_Queue *q, void *msg, OS_Ulong wait)
{
    return (osMessageQueuePut(*q, msg, 0, (uint32_t)wait) == osOK) ? OS_SUCCESS
                                                                   : (OS_Uint)1;
}
static inline OS_Uint os_queue_receive(OS_Queue *q, void *msg, OS_Ulong wait)
{
    return (osMessageQueueGet(*q, msg, 0, (uint32_t)wait) == osOK) ? OS_SUCCESS
                                                                   : (OS_Uint)1;
}

static inline OS_Uint os_event_group_create(OS_EventGroup *g, const char *name)
{
    osEventFlagsAttr_t attr = {0};
    attr.name = name;
    *g = osEventFlagsNew(&attr);
    return (*g) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_event_group_delete(OS_EventGroup *g)
{
    return (osEventFlagsDelete(*g) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_event_group_set(OS_EventGroup *g, OS_Ulong flags)
{
    return (osEventFlagsSet(*g, (uint32_t)flags) != osFlagsError) ? OS_SUCCESS
                                                                  : (OS_Uint)1;
}
static inline OS_Uint os_event_group_clear(OS_EventGroup *g, OS_Ulong flags)
{
    return (osEventFlagsClear(*g, (uint32_t)flags) != osFlagsError) ? OS_SUCCESS
                                                                    : (OS_Uint)1;
}
static inline OS_Uint os_event_group_get(OS_EventGroup *g, OS_Ulong flags, OS_Uint option,
                                         OS_Ulong *actual, OS_Ulong wait)
{
    const uint32_t opt = (option == OS_AND) ? osFlagsWaitAll : osFlagsWaitAny;
    const uint32_t got = osEventFlagsWait(*g, (uint32_t)flags, opt, (uint32_t)wait);
    if (actual) {
        *actual = (OS_Ulong)got;
    }
    return (got & osFlagsError) ? (OS_Uint)1 : OS_SUCCESS;
}

typedef struct {
    void (*cb)(OS_Ulong);
    OS_Ulong id;
} os_cmsis_timer_cb_t;

static inline void os_cmsis_timer_entry(void *arg)
{
    os_cmsis_timer_cb_t *params = (os_cmsis_timer_cb_t *)arg;
    params->cb(params->id);
}

static inline OS_Uint os_timer_create(OS_Timer *t, const char *name,
                                      void (*cb)(OS_Ulong), OS_Ulong id,
                                      OS_Ulong initial, OS_Ulong reload,
                                      OS_Uint auto_act)
{
    os_cmsis_timer_cb_t *params =
        (os_cmsis_timer_cb_t *)malloc(sizeof(os_cmsis_timer_cb_t));
    if (params == 0) {
        return (OS_Uint)1;
    }
    params->cb = cb;
    params->id = id;
    osTimerAttr_t attr = {0};
    attr.name = name;
    const osTimerType_t type = (reload != 0u) ? osTimerPeriodic : osTimerOnce;
    *t = osTimerNew(os_cmsis_timer_entry, type, params, &attr);
    if (*t == 0) {
        free(params);
        return (OS_Uint)1;
    }
    if (auto_act) {
        return (osTimerStart(*t, (uint32_t)initial) == osOK) ? OS_SUCCESS : (OS_Uint)1;
    }
    (void)initial;
    return OS_SUCCESS;
}
static inline OS_Uint os_timer_delete(OS_Timer *t)
{
    return (osTimerDelete(*t) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_timer_activate(OS_Timer *t)
{
    return (osTimerStart(*t, 1u) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}
static inline OS_Uint os_timer_deactivate(OS_Timer *t)
{
    return (osTimerStop(*t) == osOK) ? OS_SUCCESS : (OS_Uint)1;
}

static inline OS_Uint os_stream_buffer_create(OS_StreamBuffer *b, size_t capacity,
                                              size_t trigger)
{
    (void)capacity;
    (void)trigger;
    *b = 0;
    return OS_SUCCESS;
}
static inline OS_Uint os_stream_buffer_delete(OS_StreamBuffer *b)
{
    *b = 0;
    return OS_SUCCESS;
}
static inline OS_Uint os_stream_buffer_send(OS_StreamBuffer *b, const void *data,
                                            size_t len, OS_Ulong wait)
{
    (void)b;
    (void)data;
    (void)len;
    (void)wait;
    return (OS_Uint)1;
}
static inline OS_Uint os_stream_buffer_receive(OS_StreamBuffer *b, void *data,
                                               size_t len, OS_Ulong wait)
{
    (void)b;
    (void)data;
    (void)len;
    (void)wait;
    return (OS_Uint)1;
}

typedef uint8_t OS_Critical;
extern OS_Critical os_critical_mux;
static inline void os_critical_enter(void)
{
    (void)osKernelLock();
}
static inline void os_critical_exit(void)
{
    (void)osKernelUnlock();
}

#ifdef __cplusplus
}
#endif
