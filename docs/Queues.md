[⬅️ Previous](Synchronization.md) | [🗂️ Index](index.md) | [➡️ Next](GenericTemplates.md)

# Message Queues

`OsQueue<MessageType, Capacity>` is a typed, fixed-capacity queue backed by a
FreeRTOS queue handle. The queue object is **eagerly constructed** in its
constructor — there is no lazy-init dance and no extra wrapping mutex
(FreeRTOS queues are already MT-safe).

## Highlights

- **Type-safe:** message size is derived from `sizeof(T)` at compile time.
- **Modern API:** `Send` / `Receive` take `uint32_t timeout_ms` (use `UINT32_MAX`
  to wait forever). No `OS_Ulong` / `OS_WAIT_FOREVER` leakage.
- **Inline storage:** capacity is fixed by the template parameter; no heap
  allocation in the hot path.
- **Validation:** `IsValid()` confirms the underlying handle was created.

## Public surface

```cpp
template <typename MessageType, std::size_t kCapacity>
class OsQueue {
public:
    explicit OsQueue(const char* name) noexcept;

    bool Send(const MessageType& msg,
              uint32_t timeout_ms = UINT32_MAX) noexcept;

    bool Receive(MessageType& out,
                 uint32_t timeout_ms = UINT32_MAX) noexcept;

    [[nodiscard]] bool        IsValid()  const noexcept;
    [[nodiscard]] std::size_t Capacity() const noexcept;
};
```

## Quick example

```cpp
#include "OsQueue.h"

OsQueue<int, /*Capacity=*/64> q{"Numbers"};

q.Send(42);                                  // blocks forever (UINT32_MAX)

int v;
if (q.Receive(v, /*timeout_ms=*/100)) {
    // got value within 100 ms
}
```

[⬅️ Previous](Synchronization.md) | [🗂️ Index](index.md) | [➡️ Next](GenericTemplates.md)
