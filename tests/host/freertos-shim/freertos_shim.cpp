/* Host implementation of the FreeRTOS shim declared in FreeRTOS.h. */
#include "freertos_shim.h"

#include <cstdlib>
#include <cstring>

struct hf_shim_queue {
    UBaseType_t length;
    UBaseType_t item_size;
    uint8_t* storage;
    bool owns_storage;
    UBaseType_t head;
    UBaseType_t count;
};

struct hf_shim_event_group {
    EventBits_t bits;
};

namespace shim {

Record& State() {
    static Record r;
    return r;
}

void Reset() { State() = Record{}; }

}  // namespace shim

extern "C" {

void* pvPortMalloc(size_t size) { return std::malloc(size); }
void vPortFree(void* p) { std::free(p); }

static QueueHandle_t NewQueue(UBaseType_t length, UBaseType_t item_size, uint8_t* storage, bool owns) {
    auto* q = new hf_shim_queue{length, item_size, storage, owns, 0, 0};
    return q;
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size) {
    auto& s = shim::State();
    ++s.queue_creates;
    ++s.heap_queue_creates;
    s.last_queue = {false, length, item_size, nullptr, nullptr};
    if (length == 0 || item_size == 0) return nullptr;
    return NewQueue(length, item_size, static_cast<uint8_t*>(std::calloc(length, item_size)), true);
}

QueueHandle_t xQueueCreateStatic(UBaseType_t length, UBaseType_t item_size, uint8_t* storage,
                                 StaticQueue_t* control) {
    auto& s = shim::State();
    ++s.queue_creates;
    s.last_queue = {true, length, item_size, storage, control};
    if (length == 0 || item_size == 0 || storage == nullptr || control == nullptr) return nullptr;
    std::memset(control, 0, sizeof(StaticQueue_t));  // the kernel initialises the control block
    return NewQueue(length, item_size, storage, false);
}

void vQueueDelete(QueueHandle_t q) {
    if (q == nullptr) return;
    if (q->owns_storage) std::free(q->storage);
    delete q;
}

BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t wait) {
    shim::State().last_queue_wait = wait;
    if (q->count == q->length) return errQUEUE_FULL;
    const UBaseType_t slot = (q->head + q->count) % q->length;
    std::memcpy(q->storage + slot * q->item_size, item, q->item_size);
    ++q->count;
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t wait) {
    shim::State().last_queue_wait = wait;
    if (q->count == 0) return errQUEUE_EMPTY;
    std::memcpy(item, q->storage + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->length;
    --q->count;
    return pdPASS;
}

EventGroupHandle_t xEventGroupCreate(void) { return new hf_shim_event_group{0}; }
void vEventGroupDelete(EventGroupHandle_t g) { delete g; }

EventBits_t xEventGroupSetBits(EventGroupHandle_t g, EventBits_t bits) {
    g->bits |= bits;
    return g->bits;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t g, EventBits_t bits) {
    const EventBits_t before = g->bits;
    g->bits &= ~bits;
    return before;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t g, EventBits_t bits, BaseType_t clear_on_exit,
                                BaseType_t wait_for_all, TickType_t wait) {
    auto& s = shim::State();
    s.last_event_wait = wait;
    s.last_event_clear = clear_on_exit;
    s.last_event_all = wait_for_all;
    const EventBits_t seen = g->bits;
    const bool met = wait_for_all ? ((seen & bits) == bits) : ((seen & bits) != 0);
    if (met && clear_on_exit) g->bits &= ~bits;
    return seen;  // on a timeout the kernel also returns the current bits
}

/* Declared so the backend compiles; not modelled. */
BaseType_t xTaskCreate(TaskFunction_t, const char*, uint32_t, void*, UBaseType_t, TaskHandle_t*) { return pdFAIL; }
void vTaskSuspend(TaskHandle_t) {}
void vTaskResume(TaskHandle_t) {}
void vTaskDelete(TaskHandle_t) {}
void vTaskDelay(TickType_t) {}
eTaskState eTaskGetState(TaskHandle_t) { return eInvalid; }
BaseType_t xTaskGetSchedulerState(void) { return taskSCHEDULER_NOT_STARTED; }
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return nullptr; }
TickType_t xTaskGetTickCount(void) { return 0; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) { return 0; }
void vTaskPrioritySet(TaskHandle_t, UBaseType_t) {}
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return nullptr; }
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void) { return nullptr; }
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t, UBaseType_t) { return nullptr; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdFAIL; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdFAIL; }
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t, TickType_t) { return pdFAIL; }
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t) { return pdFAIL; }
void vSemaphoreDelete(SemaphoreHandle_t) {}
UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t) { return 0; }
TimerHandle_t xTimerCreate(const char*, TickType_t, UBaseType_t, void*, TimerCallbackFunction_t) { return nullptr; }
void* pvTimerGetTimerID(TimerHandle_t) { return nullptr; }
BaseType_t xTimerStart(TimerHandle_t, TickType_t) { return pdFAIL; }
BaseType_t xTimerStop(TimerHandle_t, TickType_t) { return pdFAIL; }
BaseType_t xTimerDelete(TimerHandle_t, TickType_t) { return pdFAIL; }
StreamBufferHandle_t xStreamBufferCreate(size_t, size_t) { return nullptr; }
void vStreamBufferDelete(StreamBufferHandle_t) {}
size_t xStreamBufferSend(StreamBufferHandle_t, const void*, size_t, TickType_t) { return 0; }
size_t xStreamBufferReceive(StreamBufferHandle_t, void*, size_t, TickType_t) { return 0; }

}  // extern "C"
