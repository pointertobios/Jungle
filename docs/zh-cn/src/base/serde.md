# Serde 序列化反序列化框架

Jungle 的 `serde` 是一个基于 C++26 反射的序列化/反序列化框架。它不依赖宏或外部代码生成——所有类型信息在编译期通过反射获取。

## 接口、行为与约束

### 核心概念

框架将"如何表示数据"与"数据本身是什么"解耦：

- **Target**（序列化目标）：接收类型安全的数据写入调用，产生某种输出格式
- **Source**（反序列化源）：从某种输入格式读取数据，写入已有变量
- **Customizer**（定制器）：在序列化/反序列化过程中对特定字段的值做变换

### 类型分派顺序

序列化和反序列化的自由函数按以下顺序对类型做编译期分派：

1. `bool`
2. `std::integral`（整数类型）
3. `std::floating_point`（浮点类型）
4. `enum`
5. `std::optional<T>`
6. `std::ranges::range`（容器/范围类型）
7. `class` / `struct`

如果一个类型不匹配以上任何类别，编译期 `static_assert` 报错。

---

### 序列化

#### 自由函数

```cpp
// 写入已有 target
template<SerializeTargetImpl Target, typename T>
void serialize(const T &value, Target &target);

// 创建 target 并返回结果
template<SerializeTargetImpl Target, typename T>
typename Target::target_type serialize(T &&value);
```

`serialize(value, target)` 将 `value` 写入 `target`。便捷重载 `serialize<Target>(value)` 默认构造 `Target`、调用上述函数、并通过 `target.deliver_result()` 返回最终产物。

#### SerializeTarget 基类

所有 Target 必须继承 `SerializeTarget<Target>`（CRTP），并满足 `SerializeTargetImpl` concept：

- 定义 `target_type`（最终产物的类型）
- 实现 `deliver_result() -> target_type`
- 实现 `spawn_subtarget() -> Target`（创建子 target，用于嵌套结构）
- 实现各类型的 `serialize_*` 方法（基类通过 CRTP 调用）

基类 `SerializeTarget` 提供的模板方法及其 requires 约束：

| 基类方法                                  | 调用的派生类方法                                                                                                                                                              |
| ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `serialize_bool(const bool &)`            | `serialize_bool(const bool &) -> void`                                                                                                                                        |
| `serialize_integral(const I &)`           | `serialize_integral(const I &) -> void`                                                                                                                                       |
| `serialize_floating_point(const F &)`     | `serialize_floating_point(const F &) -> void`                                                                                                                                 |
| `serialize_enum(const T &)`               | `serialize_enum(const T &) -> void`                                                                                                                                           |
| `serialize_optional(const optional<T> &)` | `serialize_optional_nonnull() -> void`、`serialize_optional_nullopt() -> void`                                                                                                |
| `serialize_range(const R &)`              | `serialize_range_head() -> void`、`serialize_range_element_end() -> void`、`serialize_range_tail(usize) -> void`                                                              |
| `serialize_class_object(const T &)`       | `serialize_class_head(string_view) -> void`、`serialize_class_field(string_view) -> void`、`serialize_class_field_end() -> void`、`serialize_class_tail(string_view) -> void` |

#### class 的序列化流程

对于 class 类型，框架使用反射遍历所有非静态数据成员：

- 不含 `[[=customized]]` 注解：所有非静态数据成员均被序列化
- 含 `[[=customized]]` 注解：仅带 `[[=field]]` 标记的成员被序列化
- 对于带 `[[=customize<C>]]` 注解的成员，由定制器 `C` 的 `serialize` 方法接管该字段的输出

反射使用 `std::meta::access_context::unchecked()` 访问私有成员。

---

### 反序列化

#### 自由函数

```cpp
// 直接操作 source，写入已有变量
template<typename T, DeserializeSourceImpl Source>
void deserialize(Source &source, T &value);

// 从 payload 构造新值并返回（T 需 default_constructible）
template<typename T, DeserializeSourceImpl Source>
  requires std::is_default_constructible_v<T>
T deserialize(const typename Source::source_type &source_payload);

// 从 payload 写入已有变量
template<typename T, DeserializeSourceImpl Source>
void deserialize(const typename Source::source_type &source_payload, T &value);
```

第一个重载是最通用的形式：`T` 的构造由调用方负责，source 只负责填充。后两个是便捷重载。

#### DeserializeSource 基类

所有 Source 必须继承 `DeserializeSource<Source>`（CRTP），并满足 `DeserializeSourceImpl` concept：

- 定义 `source_type`（原始输入类型）
- 实现 `provide_source(const source_type &) -> void`
- 实现 `spawn_subsource() -> Source`
- 实现各类型的 `deserialize_*` 方法

基类 `DeserializeSource` 提供的模板方法及其 requires 约束：

| 基类方法                            | 调用的派生类方法                                                                                                                                                   |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `deserialize_bool(bool &)`          | `deserialize_bool(bool &) -> void`                                                                                                                                 |
| `deserialize_integral(I &)`         | `deserialize_integral(I &) -> void`                                                                                                                                |
| `deserialize_floating_point(F &)`   | `deserialize_floating_point(F &) -> void`                                                                                                                          |
| `deserialize_enum(T &)`             | `deserialize_enum(T &) -> void`                                                                                                                                    |
| `deserialize_optional(OptionalT &)` | `deserialize_optional_nonnull() -> bool`                                                                                                                           |
| `deserialize_range(R &)`            | `deserialize_range_head() -> void`、`deserialize_range_has_element() -> bool`、`deserialize_range_element_end() -> void`、`deserialize_range_tail() -> void`       |
| `deserialize_class_object(T &)`     | `deserialize_class_head() -> string_view`、`deserialize_class_field() -> string_view`、`deserialize_class_field_end() -> void`、`deserialize_class_tail() -> void` |

> **注意**：`deserialize` 自由函数要求 `T` 在调用前已被构造。Source 的方法只负责写入，不负责构造。对于 `std::optional<T>`，内部值类型 `T` 还需满足 `default_constructible` 和 `move_constructible`。

#### range 的反序列化流程

```
deserialize_range_head()         → 消费起始标记（如 "["）
while deserialize_range_has_element():
    spawn_subsource()            → 创建子 source
    deserialize(subsource, elem) → 递归反序列化元素
    deserialize_range_element_end() → 消费分隔符（如 ","）
deserialize_range_tail()         → 消费结束标记（如 "]"）
```

#### class 的反序列化流程

与序列化对称：`deserialize_class_head()` 读取类型名，反射遍历成员，`deserialize_class_field()` 读取字段名，通过子 source 递归反序列化字段值，`deserialize_class_field_end()` 消费分隔符，`deserialize_class_tail()` 消费结束标记。

定制器在反序列化时的调用：

```cpp
value.[:m:] = customizer_instance.deserialize(value.[:m:], subsource);
```

定制器接收成员引用和子 source，负责读取并返回变换后的值。

---

### 注解

| 注解                | 用途                                                            |
| ------------------- | --------------------------------------------------------------- |
| `[[=customized]]`   | 标注 class 为定制序列化——只有 `[[=field]]` 成员参与             |
| `[[=field]]`        | 标记某一成员参与序列化（仅在 `[[=customized]]` class 中有意义） |
| `[[=customize<C>]]` | 指定字段级定制器 `C`，该字段的序列化/反序列化由 `C` 接管        |

注解可以组合使用：

```cpp
struct [[= customized]] S {
  [[= field]] int normal = 1;
  [[= field]] [[= customize<PlusThousand>]] int boosted = 100;
  int skipped = 999;   // 无 [[=field]]，不参与序列化
};
```

---

### Customizer 概念

```cpp
template<template<typename> typename Custr>
concept Customizer =
  std::is_default_constructible_v<Custr<int>> &&
  requires(Custr<int> c, int value, detail::TraitTargetSource &target) {
    { c.serialize(value, target) }      -> std::same_as<void>;
    { c.deserialize(value, target) }    -> std::same_as<int>;
  };
```

定制器是一个**单参数模板**：`template<typename T> struct MyCustomizer { ... }`。框架会为每个使用该定制器的字段实例化 `MyCustomizer<字段类型>`。

- `serialize(const T &value, auto &target)`：将 `value` 经过变换后写入 `target`
- `deserialize(T &value, auto &source)`：从 `source` 读取原始值到 `value`，返回变换后的值

定制器的 `deserialize` 返回类型应与字段类型一致。

---

## 实现 Source/Target 与 Customizer

### 实现 TextTarget

以下是一个将数据序列化为可读文本格式的 Target 实现（位于单元测试目录，仅作参考）：

```cpp
class TextTarget : public SerializeTarget<TextTarget> {
public:
    using target_type = ustr;

    TextTarget() = default;
    target_type deliver_result() { return std::move(m_result); }

    // spawn_subtarget 让嵌套结构共享同一个输出缓冲区
    TextTarget spawn_subtarget() { return TextTarget{m_result}; }

    void serialize_bool(const bool &value) {
        m_result.append(value ? "true" : "false");
    }

    template<std::integral I>
    void serialize_integral(const I &value) {
        m_result.append(ustr::format("{}", value));
    }

    template<std::floating_point F>
    void serialize_floating_point(const F &value) {
        m_result.append(ustr::format("{}", value));
    }

    template<concepts::is_enum E>
    void serialize_enum(const E &value) {
        m_result.append(debug(value));
    }

    void serialize_optional_nonnull()  { m_result.append("optional##"); }
    void serialize_optional_nullopt()  { m_result.append("optional##nullopt"); }
    void serialize_range_head()        { m_result.append("["); }
    void serialize_range_element_end() { m_result.append(","); }
    void serialize_range_tail(usize)   { m_result.append("]"); }
    void serialize_class_head(std::string_view ident) {
        m_result.append(ustr::format("{}{{", ident));
    }
    void serialize_class_field(std::string_view ident) {
        m_result.append(ustr::format("{}:", ident));
    }
    void serialize_class_field_end()   { m_result.append(","); }
    void serialize_class_tail(std::string_view) { m_result.append("}"); }

private:
    TextTarget(ustr &external) : m_storage{std::nullopt}, m_result{external} {}
    std::optional<ustr> m_storage{ustr{}};
    ustr &m_result{*m_storage};
};

static_assert(SerializeTargetImpl<TextTarget>);
```

关键设计点：
- `spawn_subtarget()` 通过引用共享底层缓冲区，子 target 的输出追加到同一字符串末尾
- 使用 `ustr::format` 格式化数值，`debug()` 格式化枚举
- 各 `serialize_*` 方法的输出格式需与对应 `TextSource` 的 `deserialize_*` 解析逻辑保持一致

### 实现 TextSource

```cpp
class TextSource : public DeserializeSource<TextSource> {
public:
    using source_type = ustr;

    TextSource() = default;
    void provide_source(const source_type &source) {
        m_source = source.view();
        *m_cursor = 0;
    }
    TextSource spawn_subsource() { return TextSource{m_source, m_cursor}; }

    void deserialize_bool(bool &value) { /* 解析 "true" / "false" */ }
    template<std::integral I>
    void deserialize_integral(I &value) { /* from_chars 解析整数 */ }
    template<std::floating_point F>
    void deserialize_floating_point(F &value) { /* from_chars 解析浮点 */ }
    template<concepts::is_enum E>
    void deserialize_enum(E &value) { /* 按名字匹配枚举项 */ }

    bool deserialize_optional_nonnull() { /* 检查 "optional##" 后是否跟 "nullopt" */ }
    void deserialize_range_head()        { /* 消费 "[" */ }
    bool deserialize_range_has_element() { /* 检查下个字符是否为 "]" */ }
    void deserialize_range_element_end() { /* 消费 "," */ }
    void deserialize_range_tail()        { /* 消费 "]" */ }
    std::string_view deserialize_class_head()  { /* 消费 "TypeName{" */ }
    std::string_view deserialize_class_field() { /* 消费 "fieldName:" */ }
    void deserialize_class_field_end()   { /* 消费 "," */ }
    void deserialize_class_tail()        { /* 消费 "}" */ }

private:
    TextSource(std::string_view source, usize *cursor)
        : m_source{source}, m_cursor{cursor} {}
    std::string_view m_source;
    usize m_owned_cursor{0};
    usize *m_cursor{&m_owned_cursor};
    // 游标推进、前缀匹配等辅助方法省略
};

static_assert(DeserializeSourceImpl<TextSource>);
```

关键设计点：
- `spawn_subsource()` 通过共享游标实现：子 source 读取时自动推进父 source 的位置
- 结构方法（`head`、`has_element`、`element_end`、`tail`）负责消费格式标记
- 值方法（`deserialize_bool` 等）只负责解析值本身，不处理外围标记

### 实现 Customizer

以下定制器在序列化时对值 +1000，反序列化时 -1000：

```cpp
template<typename T>
struct PlusThousand {
    void serialize(const T &value, auto &target) const {
        target.serialize_integral(value + 1000);
    }
    template<typename U>
    U deserialize(U &value, auto &source) const {
        source.template deserialize_integral<U>(value);
        return value - 1000;
    }
};

static_assert(Customizer<PlusThousand>);
```

使用方法：

```cpp
struct S {
    [[= customize<PlusThousand>]] int score = 42;
};
```

定制器必须满足：
- 是一个单参数模板（`template<typename> struct`）
- `serialize` 接收 `const T &` 和 target 引用，返回 `void`
- `deserialize` 接收 `T &` 和 source 引用，返回 `T`（变换后的值）
- `deserialize` 内部调用 source 的值写入方法（如 `deserialize_integral`）获取原始值

### 使用示例

```cpp
// 序列化
ustr text = serialize<TextTarget>(my_object);

// 反序列化（构造新值）
auto obj = deserialize<MyStruct, TextSource>(text);

// 反序列化（写入已有变量）
MyStruct obj;
deserialize<MyStruct, TextSource>(text, obj);

// 反序列化（直接操作 source）
TextSource src;
src.provide_source(text);
deserialize(src, obj);
```
