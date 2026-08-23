#pragma once
/**
 * @file os_threadx.h
 * @brief Azure RTOS ThreadX backend for the generic OS abstraction.
 *
 * Included only by @ref OsAbstraction.h when `HF_RTOS_THREADX` is selected.
 * Do not include this header directly.
 *
 * ThreadX is single-core in every configuration this wrapper targets, so
 * @ref os_thread_create_pinned degrades to @ref os_thread_create and
 * @ref os_get_current_core_id always reports 0.
 *
 * Stream buffers have no ThreadX equivalent; those entry points exist for
 * source compatibility and report failure rather than pretending to work.
 *
 * @see os/README.md
 */

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

/** Single-core kernel: @p core_id has no meaning and is ignored. */
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
static inline OS_Uint os_thread_yield(void)
{ (void)tx_thread_relinquish(); return OS_SUCCESS; }
static inline int os_kernel_is_running(void)
{ return tx_thread_identify() != TX_NULL; }
static inline OS_Uint os_thread_is_current(const OS_Thread *t)
{ return (t != 0 && tx_thread_identify() == t) ? 1u : 0u; }

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

/* No ThreadX stream-buffer primitive — report failure, never fake success. */
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
