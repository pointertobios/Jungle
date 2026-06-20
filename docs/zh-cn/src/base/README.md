# Jungle 基础库

`jungle-base` 是 Jungle 引擎的基础设施层，提供类型系统、序列化框架、反射工具、测试框架等底层能力。所有上层模块（`jungle-core`、`jungle-server` 等）均依赖本库。

## 模块结构

```
include/jungle/
├── concepts.h        # 概念约束（Debug、is_enum 等）
├── debug.h           # 基于反射的通用 debug 输出
├── meta.h            # 反射元编程工具
├── panic.h           # 不可恢复错误处理
├── preusing.h        # 常用类型别名集中引入
├── container/        # 容器
│   ├── hash_map.h    # 自定义哈希映射
│   └── mpsc.h        # MPSC 无锁有界队列
├── serde/            # 序列化反序列化框架
│   ├── serde.h       # 概念定义与注解
│   ├── serialize.h   # SerializeTarget 基类与 serialize() 自由函数
│   └── deserialize.h # DeserializeSource 基类与 deserialize() 自由函数
├── test/             # 测试框架
│   └── test.h        # JUNGLE_SYNC_TEST 宏与断言
├── types/            # 基础类型
│   ├── int.h         # 固定宽度整数别名（u8, i32, usize 等）
│   └── uchar.h       # Unicode 字符与字符串（uchar, ustr）
└── util/             # 工具
    ├── murmur.h      # MurmurHash 哈希
    ├── parse.h       # Base64 编码视图
    ├── type_id.h     # 编译期类型 ID
    └── types.h       # 类型特征工具
```

## 编译要求

- C++23 标准（启用了 `-std=c++23`）
- 启用反射扩展（`-freflection`）
- 启用契约（`-fcontracts`）
- 禁用 RTTI 和异常（`-fno-rtti -fno-exceptions`）

## 关键组件

### 类型系统

`types/int.h` 定义了固定宽度的整数别名，与 `std` 对应：

| 别名          | 对应类型                          |
| ------------- | --------------------------------- |
| `u8` ～ `u64` | `std::uint8_t` ～ `std::uint64_t` |
| `i8` ～ `i64` | `std::int8_t` ～ `std::int64_t`   |
| `usize`       | `std::size_t`                     |
| `isize`       | `std::ptrdiff_t`                  |

`types/uchar.h` 提供了 `uchar`（Unicode 码点）和 `ustr`（UTF-8 字符串），支持 `std::format`。

### 反射工具 (`meta.h`)

基于 C++26 反射提供编译期类型内省：

- `has_annotation()` — 检查类型/成员是否有某注解
- `has_template_annotation()` — 检查类型/成员是否有某模板注解的实例
- `nth_template_annotation_argument_of()` — 获取模板注解的第 N 个参数
- `is_specialization_of_template()` — 判断是否为某模板的实例
- `nonstatic_data_members_with_annotation()` — 获取带某注解的所有非静态数据成员

### 序列化框架 (`serde/`)

基于反射的类型安全序列化/反序列化框架。详见 [序列化反序列化框架](./serde.md)。

核心特性：
- 零宏、零外部代码生成——完全依赖 C++26 编译期反射
- CRTP 设计：实现 `SerializeTarget` 或 `DeserializeSource` 即可接入
- 通过注解控制字段参与策略（`[[=customized]]` / `[[=field]]`）
- 支持字段级定制器（`[[=customize<C>]]`）
- 递归处理嵌套 struct、容器、`std::optional`

### 测试框架 (`test/`)

轻量级同步测试框架：

```cpp
JUNGLE_SYNC_TEST(my_test) {
    JUNGLE_SYNC_ASSERT(1 + 1 == 2, "basic arithmetic should work");
    JUNGLE_SYNC_SUCCESS();
}
```

通过 `ctest` 集成运行。

### 错误处理 (`panic.h`)

禁用异常的环境下使用 `panic()` 处理不可恢复错误，支持格式化消息。

### 调试输出 (`debug.h`)

基于反射的通用 `debug()` 函数，支持的类别覆盖基础类型、枚举、范围、class/struct（含私有成员）。
