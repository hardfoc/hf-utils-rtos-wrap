#pragma once
/**
 * @file os_none.h
 * @brief No-scheduler backend for the generic OS abstraction.
 *
 * Included only by @ref OsAbstraction.h when no RTOS is selected. Do not
 * include this header directly.
 *
 * Used by bare-metal images and by host unit tests, where the caller drives
 * work cooperatively instead of relying on a scheduler. Object creation
 * succeeds so construction paths stay linear; anything that would require a
 * scheduler to make progress (receiving from an empty queue, a stream-buffer
 * read) reports failure rather than pretending to have data.
 *
 * @see os/README.md
 */

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
/** No scheduler and no cores — @p core_id is ignored. */
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
static inline OS_Uint os_thread_yield(void)             { return OS_SUCCESS; }
static inline int os_kernel_is_running(void)            { return 0; }
static inline OS_Uint os_thread_is_current(const OS_Thread *t)
{ (void)t; return 0; }

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

/* Queue — creation succeeds; a receive has nothing to hand back */
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
