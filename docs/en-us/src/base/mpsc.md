# MPSC Lock-Free Bounded Queue

`jungle::container::mpsc<T>` is a multiple-producer single-consumer (MPSC) lock-free bounded queue. It is built on a ring buffer with atomic operations for data-race-free concurrency.

## Feature Summary

- **Lock-free**: Both send and receive use only `std::atomic`, with no mutexes
- **Multiple producers**: `sender` is freely copyable; concurrent sends from multiple threads are safe
- **Single consumer**: `receiver` is non-copyable; only one thread should call `recv()` at a time
- **Bounded capacity**: Minimum capacity specified at creation, internally rounded up to the next power of two
- **Type compatible**: Supports move-only, copy-only, and trivial types (dispatched automatically via `try_move_t`)
- **No exceptions / no RTTI**: Consistent with Jungle's overall design

## Creating a Queue

Use the static factory method `queue()` to create a `sender` / `receiver` pair:

```cpp
#include "jungle/container/mpsc.h"

using jungle::container::mpsc;

// Default minimum capacity of 1023
auto [sender, receiver] = mpsc<int>::queue();

// Specify minimum capacity
auto [s, r] = mpsc<std::string>::queue(8);
```

The `size` parameter specifies the **minimum** capacity guarantee. Internally, the actual capacity is rounded up to the next power of two no less than `size`, so the real number of slots may exceed the requested value.

Due to the ring buffer's full/empty discrimination requiring one reserved slot, the **effective capacity is actual capacity minus one**.

Queue storage is managed by `std::shared_ptr`; all `sender` and `receiver` instances share the same underlying buffer.

## Sending

`sender` is both copyable and movable. Multiple `sender` copies may call `send()` concurrently:

```cpp
auto [s1, receiver] = mpsc<int>::queue();
auto s2 = s1;       // sender is copyable, shares the same queue
auto s3 = s1;

s1.send(10);        // sent from thread A
s2.send(20);        // sent from thread B
s3.send(30);        // sent from thread C
```

### send Signature

```cpp
[[nodiscard]] bool send(try_move_t<T> value);
```

- The parameter type `try_move_t<T>` automatically selects the optimal passing convention:
  - Move-only types → `T&&` (move semantics)
  - Copy-only types → `const T&` (copy semantics)
  - Trivial types (`int`, etc.) → `T` (pass by value)
- Returns `true` on success, `false` if the queue is full

```cpp
if (!sender.send(data)) {
    // Queue is full; retry or drop
}
```

## Receiving

`receiver` is non-copyable, movable only. Only one thread should call `recv()` at a time:

```cpp
auto val = receiver.recv();
if (val.has_value()) {
    // process *val
}
```

### recv Signature

```cpp
[[nodiscard]] std::optional<T> recv();
```

- Returns `std::optional<T>`:
  - A value: the front element has been successfully dequeued
  - `std::nullopt`: the queue is empty

Elements are received in **FIFO** (first-in-first-out) order.

## Capacity Semantics

```cpp
auto [s, r] = mpsc<int>::queue(4);
// Actual capacity ≥ 4 (may be 7, 15, etc., depending on power-of-2 rounding)

// At least 4 sends are guaranteed to succeed
for (int i = 0; i < 4; ++i) {
    s.send(i);  // all succeed
}

// Do not assume the 5th send will fail —
// actual capacity may be larger
```

To determine when the queue is truly full, loop `send()` until it returns `false`.

## Thread Safety

| Operation       | Safety                                                      |
| --------------- | ----------------------------------------------------------- |
| `send`          | Multi-thread safe (concurrent producers)                    |
| `recv`          | Single-thread (same receiver must not be used concurrently) |
| `send` + `recv` | Safe (producers and consumer may run concurrently)          |

## Usage Examples

### Basic Send and Receive

```cpp
auto [sender, receiver] = mpsc<int>::queue();

sender.send(42);
auto val = receiver.recv();
// *val == 42
```

### Multiple Producers

```cpp
auto [s1, receiver] = mpsc<int>::queue();
auto s2 = s1;
auto s3 = s1;

s1.send(10);
s2.send(20);
s3.send(30);

// Received in send order: 10, 20, 30
receiver.recv();  // 10
receiver.recv();  // 20
receiver.recv();  // 30
```

### Move-Only Types

```cpp
auto [sender, receiver] = mpsc<std::unique_ptr<int>>::queue();

sender.send(std::make_unique<int>(42));
auto ptr = receiver.recv();
// *ptr == 42
```

### Draining the Queue

```cpp
while (auto val = receiver.recv()) {
    process(*val);
}
```

## Notes

- `receiver` must be obtained from the `queue()` return tuple; there is no default-constructed `receiver`
- Once all senders are destroyed, queue memory is freed by whichever `receiver` or `sender` last holds the shared state
- This is a **bounded** queue; it is not suitable for scenarios where producer throughput consistently exceeds consumer throughput
