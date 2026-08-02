---
name: jungle-test
description: '为 Jungle C++26 游戏引擎项目编写单元测试。触发词：写测试、单元测试、test、为 X 写测试、测试 X、add test for X。默认行为：单元测试（<module>/unit_tests/*.cpp）。仅当用户明确说"集成测试"才生成集成测试（tests/）。'
argument-hint: "模块名、类名或组件名（如 hash_map、ComponentID、rwspinlock）"
user-invocable: true
---

# Jungle 项目测试编写

## 决策树

```
用户输入
  ├─ 明确包含"集成测试" → 集成测试（tests/ 目录）
  └─ 其他一切 → 单元测试（<module>/unit_tests/ 目录）
```

## 测试框架速查

```cpp
#include "jungle/test/test.h"

JUNGLE_SYNC_TEST(describe_what_is_tested) {
    JUNGLE_SYNC_ASSERT(condition, "失败信息 {}", args...);
    JUNGLE_SYNC_SUCCESS();
}
```

- 测试名：snake_case，描述被测试的行为（如 `default_state`、`copy_and_move`）
- **单个测试函数名不需要添加测试对象名称前缀**——文件名已表明被测对象，无需在函数名中重复
- 断言消息：**中文**，描述期望行为
- 无 `try/catch`、`throw`、`dynamic_cast`、`typeid`、RTTI、异常

## 流程

### 1. 确定目标

从用户输入中提取被测试对象名称。可能的输入形式：

- "测试 hash_map" → `hash_map`
- "为 ComponentID 写单元测试" → `ComponentID`
- "rwspinlock 测试" → `rwspinlock`

### 2. 定位模块

在代码库中搜索该名称，确定它属于哪个模块：

| 模块           | 命名空间                             | 测试目录                   |
| -------------- | ------------------------------------ | -------------------------- |
| `jungle-base`  | `jungle::`                           | `jungle-base/unit_tests/`  |
| `jungle-core`  | `jungle::core::`                     | `jungle-core/unit_tests/`  |
| `jungle-tasks` | `jungle::tasks::` / `jungle::sync::` | `jungle-tasks/unit_tests/` |

搜索策略：

1. 用名称在 `include/` 目录 grep 头文件声明
2. 根据所在路径判断模块归属
3. 读头文件了解 API 接口（公开方法、类型别名、静态工厂等）

### 3. 读取已有测试作为风格参考

在开始编写之前，读取同模块下至少一个已有测试文件，确认：

- include 路径写法
- `using namespace` 惯例
- 测试命名风格
- 断言消息风格

### 4. 设计测试用例

覆盖以下维度（按优先级）：

1. **默认构造/初始状态** — 刚创建的对象是否处于预期状态
2. **基本操作成功路径** — 核心 API 的正常使用
3. **边界条件** — 空值、零值、极限值
4. **错误/失败路径** — 操作失败时的行为
5. **移动/拷贝语义** — 如果类型支持
6. **与相关类型的交互** — 组合使用场景

### 5. 编写测试文件

文件模板（模板中的注释仅用于说明，实际生成代码时除 MIT 标记外不写任何注释）：

```cpp
// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "<被测头文件路径>"
#include "jungle/test/test.h"

// 如果涉及异步，还需：
// #include "jungle/test/async_test.h"

using namespace jungle;
// 根据需要添加子命名空间

JUNGLE_SYNC_TEST(descriptive_test_name) {
    // Arrange: 准备测试数据

    // Act: 执行操作

    // Assert: 验证结果
    JUNGLE_SYNC_ASSERT(condition, "中文描述：期望的行为");
    JUNGLE_SYNC_SUCCESS();
}
```

约束：

- 文件以 MIT 开源标记开头（使用 `mit` snippet 直接生成）
- **除 MIT 开源标记外，测试代码中不编写任何注释**
- `#pragma once` **不用于** `.cpp` 测试文件
- 不使用 `try/catch`、`throw`
- **不为任何异常情况兜底**：不为可能发生的死循环和丢唤醒添加超时保护，不为有问题的实现写 hack 绕过。测试只验证正确行为，发现 bug 就让测试失败，不要掩盖问题。
- **测试必须是确定性的**：禁止含有脆弱的时序依赖。核心问题是测试结果是否依赖并发任务间不可控的执行顺序——而非某具体 API（如 `yield`）能否使用。例如，`yield()` 后假定另一 worker 上的任务"已经执行到某个挂起点"就是脆弱的时序依赖；而用 `yield()` 仅让出执行权让调度器有机会推进、最终通过 `co_await jh` 等待任务完成则是安全的。对于同步原语（如 `condition_variable`、`spinlock`）的异步测试，优先使用**谓词等待**（`co_await cv(pred)`）来消除时序竞争，谓词在 `await_ready` 中即被求值：若条件已为真则立即返回不挂起，若已挂起则被 `notify` 唤醒后重新检查谓词。两种路径下测试行为均确定。

### 异步测试时序模式

`jungle-tasks` 模块的异步测试运行在多线程运行时上，`tasks::spawn` 将任务随机分配到不同 worker。

脆弱时序依赖的本质是：**测试假定 task A 已到达某个内部状态时 task B 才执行下一步，但二者在不同 worker 上并发运行，到达顺序不受控制。**

**脆弱的时序依赖**（禁止）：

```cpp
// ❌ 假定 yield 后派生任务已挂载到 cv — 不同 worker 上不成立
auto jh = tasks::spawn([&]() -> async::future<> {
    co_await cv();
});
co_await this_task::yield();  // 派生任务可能在另一 worker 上，尚未执行
cv.notify_one();               // 可能先于 await_suspend，丢唤醒
```

```cpp
// ❌ 无条件等待 + 依赖时序 — 同样的问题
auto jh = tasks::spawn([&]() -> async::future<> {
    co_await cv();  // 无条件挂起
});
co_await this_task::yield();  // 不保证任务已挂载到 cv
cv.notify_one();               // 可能先于 await_suspend 执行
```

**消除时序依赖**（正确）：

```cpp
// ✅ 谓词等待：无论 notify 先于或后于挂载，行为一致
JUNGLE_ASYNC_TEST(example_predicate_wait) {
    condition_variable cv;
    bool ready = false;
    bool done = false;

    auto jh = tasks::spawn([&]() -> async::future<> {
        co_await cv([&] { return ready; });  // await_ready 即检查谓词
        done = true;
    });

    ready = true;
    cv.notify_one();

    co_await jh;
    JUNGLE_ASYNC_ASSERT(done, "等待者应完成执行");
    JUNGLE_ASYNC_SUCCESS();
}
```

`yield()` 和 `sleep()` 本身并非禁止使用——禁止的是让测试结果依赖不可控的任务间并发执行顺序。`yield()` 用于让出执行权、`co_await jh` 用于等待任务完成，这些是安全且必要的同步手段。

### 6. 注册到 CMake

在模块的 `CMakeLists.txt` 中找到 `JUNGLE_UNIT_TEST_SOURCES` 列表，添加新的 `.cpp` 文件。

链接依赖参考：

- `jungle-base` → `target_link_libraries(... PUBLIC jungle::test)`
- `jungle-core` → `target_link_libraries(... PUBLIC jungle::test jungle::core)`
- `jungle-tasks` → `target_link_libraries(... PUBLIC jungle::test-async)`（异步测试需 `jungle::test-async`）

### 7. 构建与运行

**禁止主动构建或运行测试**：除非用户明确要求（如"构建并运行"、"跑一下测试"），否则仅生成代码和修改 CMakeLists.txt，不执行任何构建或运行命令。

当用户明确要求构建或运行时：

- **必须使用 CMake Tools 扩展**（`Build_CMakeTools` / `RunCtest_CMakeTools` 工具），不得回退到终端命令（`run_in_terminal`）。
- 如果 CMake Tools 扩展不可用，明确告知用户无法执行构建/测试，不给出手动命令行替代方案。

## 集成测试（仅当用户明确要求）

- 文件位置：`tests/<name>.cpp`
- 在 `tests/CMakeLists.txt` 中注册：`add_executable` + `add_test`
- 使用 `int main()` 入口，可以自由组合多个测试操作
- 集成测试可以跨模块依赖

## 注意事项

- **编译器**：仅 GCC + C++26 扩展（`-freflection`、`-fcontracts`）
- **字符串**：主要使用 `jungle::ustr`
- **容器**：优先使用 `jungle::hash_map` 而非 `std::unordered_map`
- **固定宽度整数**：使用 `jungle::types/int.h` 中的类型
