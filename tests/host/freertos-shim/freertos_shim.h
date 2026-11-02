/* What the FreeRTOS shim recorded, for the tests to check. */
#ifndef HF_TEST_FREERTOS_SHIM_STATE_H_
#define HF_TEST_FREERTOS_SHIM_STATE_H_

#include <cstddef>
#include <cstdint>

#include "FreeRTOS.h"

namespace shim {

struct QueueCreate {
    bool is_static{false};
    UBaseType_t length{0};
    UBaseType_t item_size{0};
    const void* storage{nullptr};  ///< item storage handed to xQueueCreateStatic
    const void* control{nullptr};  ///< StaticQueue_t handed to xQueueCreateStatic
};

struct Record {
    int queue_creates{0};
    int heap_queue_creates{0};
    QueueCreate last_queue{};
    TickType_t last_queue_wait{0};
    TickType_t last_event_wait{0};
    BaseType_t last_event_clear{pdFALSE};
    BaseType_t last_event_all{pdFALSE};
};

Record& State();
void Reset();

}  // namespace shim

#endif
