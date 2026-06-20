# MPSC 无锁有界队列

`jungle::container::mpsc<T>` 是一个多生产者单消费者（MPSC）无锁有界队列。它基于环形缓冲区实现，使用原子操作保证无数据竞争的并发安全。

## 特性摘要

- **无锁设计**：发送和接收均无互斥锁，仅依赖 `std::atomic` 操作
- **多生产者安全**：`sender` 可自由拷贝，多线程并发发送安全
- **单消费者**：`receiver` 不可拷贝，同一时刻只能有一个消费者
- **有界容量**：创建时指定最小容量，内部向上取整为 2 的幂
- **类型兼容**：支持可移动、仅拷贝、平凡类型（通过 `try_move_t` 自动分派）

## 创建队列

通过静态工厂方法 `queue()` 创建 `sender` / `receiver` 对：

```cpp
#include "jungle/container/mpsc.h"

using jungle::container::mpsc;

// 默认最小容量 1023
auto [sender, receiver] = mpsc<int>::queue();

// 指定最小容量
auto [s, r] = mpsc<std::string>::queue(8);
```

参数 `size` 指定**最小**容量保证。

队列存储由 `std::shared_ptr` 管理，所有 `sender` 和 `receiver` 共享同一底层缓冲区。

## 发送

`sender` 可拷贝、可移动。多个 `sender` 副本可并发调用 `send()`：

```cpp
auto [s1, receiver] = mpsc<int>::queue();
auto s2 = s1;       // sender 可拷贝，共享同一队列
auto s3 = s1;

s1.send(10);        // 从线程 A 发送
s2.send(20);        // 从线程 B 发送
s3.send(30);        // 从线程 C 发送
```

### send 签名

```cpp
[[nodiscard]] bool send(try_move_t<T> value);
```

- 参数类型 `try_move_t<T>` 自动选择最优传递方式：
  - 可移动类型 → `T&&`（移动语义）
  - 仅拷贝类型 → `const T&`（拷贝语义）
  - 基础类型（`int` 等） → `T`（按值传递）
- 返回 `true` 表示发送成功，`false` 表示队列已满

```cpp
if (!sender.send(data)) {
    // 队列已满，可选择重试或丢弃
}
```

## 接收

`receiver` 不可拷贝，仅可移动。同一时刻只应有一个线程调用 `recv()`：

```cpp
auto val = receiver.recv();
if (val.has_value()) {
    // 处理 *val
}
```

### recv 签名

```cpp
[[nodiscard]] std::optional<T> recv();
```

- 返回 `std::optional<T>`：
  - 有值：成功取出队首元素
  - `std::nullopt`：队列为空

元素以 **FIFO**（先进先出）顺序被接收。

## 容量语义

```cpp
auto [s, r] = mpsc<int>::queue(4);

// 保证至少发送 4 个元素成功
for (int i = 0; i < 4; ++i) {
    s.send(i);  // 全部成功
}

// 不应假设第 5 个必定失败
// 实际容量可能更大
```

如需确切知道队列何时满，循环发送直到 `send()` 返回 `false`。

## 线程安全

| 操作            | 安全性                           |
| --------------- | -------------------------------- |
| `send`          | 多线程安全（多生产者并发）       |
| `recv`          | 单线程（同一 receiver 不可并发） |
| `send` + `recv` | 安全（生产者与消费者可并发）     |

## 使用示例

### 基本发送与接收

```cpp
auto [sender, receiver] = mpsc<int>::queue();

sender.send(42);
auto val = receiver.recv();
// *val == 42
```

### 多生产者

```cpp
auto [s1, receiver] = mpsc<int>::queue();
auto s2 = s1;
auto s3 = s1;

s1.send(10);
s2.send(20);
s3.send(30);

// 按发送顺序接收：10, 20, 30
receiver.recv();  // 10
receiver.recv();  // 20
receiver.recv();  // 30
```

### 仅移动类型

```cpp
auto [sender, receiver] = mpsc<std::unique_ptr<int>>::queue();

sender.send(std::make_unique<int>(42));
auto ptr = receiver.recv();
// *ptr == 42
```

### 排空队列

```cpp
while (auto val = receiver.recv()) {
    process(*val);
}
```

## 注意事项

- `receiver` 必须从 `queue()` 返回的元组中获取；不存在独立的 `receiver` 默认构造
- 发送者全部销毁后，队列内存由最后一个持有共享状态的 `receiver` 或 `sender` 负责释放
- 本队列为**有界**队列，不适用于生产者速率持续大于消费者的场景
