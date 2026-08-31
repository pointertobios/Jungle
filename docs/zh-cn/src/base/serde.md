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

#### 序列化自由函数

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

所有 Target 必须继承 `SerializeTarget<Target>`（CRTP），满足 `SerializeTargetImpl` concept，且可默认构造：

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

#### 反序列化自由函数

```cpp
// 从 payload 构造新值并返回（T 需 default_constructible）
// 成功时返回值，失败时返回 unexpected(Source::error_type)
template<typename T, DeserializeSourceImpl Source>
  requires std::is_default_constructible_v<T>
std::expected<T, typename Source::error_type>
deserialize(const typename Source::source_type &source_payload);

// 从 payload 写入已有变量
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] std::expected<void, typename Source::error_type>
deserialize(const typename Source::source_type &source_payload, T &value);

// 直接操作 source，写入已有变量
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] std::expected<void, typename Source::error_type>
deserialize(Source &source, T &value);
```

所有反序列化操作都通过 `std::expected` 表示成功/失败：写入已有变量的重载返回 `expected<void, Source::error_type>`，构造新值的重载返回 `expected<T, Source::error_type>`。错误从 `deserialize_*` 层层向上传递到这些自由函数。调用方负责检查返回值。

#### DeserializeSource 基类

所有 Source 必须继承 `DeserializeSource<Source>`（CRTP），满足 `DeserializeSourceImpl` concept，且可默认构造：

- 定义 `source_type`（原始输入类型）
- 定义 `error_type`（反序列化失败时的错误类型）
- 实现 `provide_source(const source_type &) -> void`
- 实现 `spawn_subsource() -> Source`
- 实现各类型的 `deserialize_*` 方法，所有方法返回 `std::expected<void, error_type>`

基类 `DeserializeSource` 提供的模板方法及其 requires 约束（下表中 `E` 表示 `Source::error_type`）：

| 基类方法                                                      | 调用的派生类方法                                                                                                                                                                                                                         |
| ------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `deserialize_bool(bool &) -> expected<void, E>`               | `deserialize_bool(bool &) -> expected<void, E>`                                                                                                                                                                                          |
| `deserialize_integral(I &) -> expected<void, E>`              | `deserialize_integral(I &) -> expected<void, E>`                                                                                                                                                                                         |
| `deserialize_floating_point(F &) -> expected<void, E>`        | `deserialize_floating_point(F &) -> expected<void, E>`                                                                                                                                                                                   |
| `deserialize_enum(T &) -> expected<void, E>`                  | `deserialize_enum(T &) -> expected<void, E>`                                                                                                                                                                                             |
| `deserialize_optional(OptionalT &) -> expected<void, E>`      | `deserialize_optional_nonnull() -> expected<void, E>`、`deserialize_optional_nullopt() -> expected<void, E>`                                                                                                                             |
| `deserialize_range(R &) -> expected<void, E>`                 | `deserialize_range_head() -> expected<void, E>`、`deserialize_range_has_element() -> expected<void, E>`、`deserialize_range_element_end() -> expected<void, E>`、`deserialize_range_tail() -> expected<void, E>`                         |
| `deserialize_class_object(T &) -> expected<void, E>`          | `deserialize_class_head() -> expected<void, E>`、`deserialize_class_field() -> expected<void, E>`、`deserialize_class_field_end() -> expected<void, E>`、`deserialize_class_tail() -> expected<void, E>`                                 |

> 与序列化不同，反序列化的结构方法（`head`、`field`、`element_end` 等）全部返回 `expected<void, E>` 而非 `void`。当解析到非法格式时可返回 `unexpected` 使上层回退或报错，而不是通过 panic 终止进程。
>
> `deserialize_optional_nonnull` / `deserialize_optional_nullopt` 以及 `deserialize_range_has_element` 用成功表示“匹配到该情况 / 仍有元素”，用 `unexpected` 表示“不是该情况 / 没有更多元素”。基类在 optional 两条路径都失败时把后者的错误向上传递；range 则在 `has_element` 失败时结束循环并继续解析 tail。

#### range 的反序列化流程

```cpp
if (auto r = deserialize_range_head(); !r) return r;   → 失败则终止并传递错误
while deserialize_range_has_element() 成功:
    spawn_subsource() → 创建子 source
    if (auto r = deserialize(subsource, elem); !r) return r;  → 递归反序列化元素
    if (auto r = deserialize_range_element_end(); !r) return r;
if (auto r = deserialize_range_tail(); !r) return r;
```

范围类型必须支持 `insert(value.end(), value_type)`；框架按输入顺序将每个已反序列化的元素插入目标范围。

#### class 的反序列化流程

与序列化对称，但 `deserialize_class_head()` 和 `deserialize_class_field()` 仅返回 `expected<void, E>`（成功与否），不返回解析到的类型名/字段名——这些字符串在反序列化中仅作验证用途，由基类的控制流统一处理失败返回。

字段定制器直接接收成员引用和子 source，并负责原地写入成员；其返回的 `expected` 会被基类检查并向上传递：

```cpp
if (auto r = customizer_instance.deserialize(value.[:m:], subsource); !r) {
    return r;
}
```

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
    { c.deserialize(value, target) }
      -> std::same_as<std::expected<void, typename detail::TraitTargetSource::error_type>>;
  };
```

定制器是一个**单参数模板**：`template<typename T> struct MyCustomizer { ... }`。框架会为每个使用该定制器的字段实例化 `MyCustomizer<字段类型>`。

- `serialize(const T &value, auto &target)`：将 `value` 经过变换后写入 `target`，返回 `void`
- `deserialize(T &value, auto &source)`：从 `source` 读取原始值，并直接写入变换后的 `value`，返回 `std::expected<void, typename Source::error_type>`

`deserialize` 必须把内部 `deserialize_*` 调用的错误原样向上返回；框架会检查该返回值并继续向上传递。

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
    enum class error_type { mismatch };

    TextSource() = default;
    void provide_source(const source_type &source) {
        m_source = source.view();
        *m_cursor = 0;
    }
    TextSource spawn_subsource() { return TextSource{m_source, m_cursor}; }

    std::expected<void, error_type> deserialize_bool(bool &value) { /* 解析 "true"/"false" */ }
    template<std::integral I>
    std::expected<void, error_type> deserialize_integral(I &value) { /* from_chars 解析 */ }
    template<std::floating_point F>
    std::expected<void, error_type> deserialize_floating_point(F &value) { /* from_chars 解析 */ }
    template<concepts::is_enum E>
    std::expected<void, error_type> deserialize_enum(E &value) { /* 按名字匹配枚举项 */ }

    std::expected<void, error_type> deserialize_optional_nonnull() { /* 消费 "optional##"，若非 "nullopt" 则成功 */ }
    std::expected<void, error_type> deserialize_optional_nullopt() { /* 消费 "nullopt" */ }
    std::expected<void, error_type> deserialize_range_head()        { /* 消费 "[" */ }
    std::expected<void, error_type> deserialize_range_has_element() { /* 检查下个字符是否为 "]" */ }
    std::expected<void, error_type> deserialize_range_element_end() { /* 消费 "," */ }
    std::expected<void, error_type> deserialize_range_tail()        { /* 消费 "]" */ }
    std::expected<void, error_type> deserialize_class_head()        { /* 消费 "TypeName{" */ }
    std::expected<void, error_type> deserialize_class_field()       { /* 消费 "fieldName:" */ }
    std::expected<void, error_type> deserialize_class_field_end()   { /* 消费 "," */ }
    std::expected<void, error_type> deserialize_class_tail()        { /* 消费 "}" */ }

private:
    TextSource(std::string_view source, usize *cursor)
        : m_source{source}, m_cursor{cursor} {}
    std::string_view m_source;
    usize m_owned_cursor{0};
    usize *m_cursor{&m_owned_cursor};
};

static_assert(DeserializeSourceImpl<TextSource>);
```

关键设计点：

- 所有方法返回 `std::expected<void, error_type>`：解析成功返回 `{}`，格式不匹配返回 `unexpected`（不 panic）
- `spawn_subsource()` 通过共享游标实现：子 source 读取时自动推进父 source 的位置
- `deserialize_optional_nonnull()` 消费 `"optional##"` 前缀后仅 peek（不消费）`"nullopt"`；若确实为 nullopt 则返回 `unexpected`，由 `deserialize_optional_nullopt()` 消费
- `deserialize_class_head()` 和 `deserialize_class_field()` 只返回 `expected<void, error_type>`，不返回解析到的类型名/字段名——这些值在反序列化路径中未被使用

### 实现 Customizer

以下定制器在序列化时对值 +1000，反序列化时 -1000：

```cpp
template<typename T>
struct PlusThousand {
    void serialize(const T &value, auto &target) const {
        target.serialize_integral(value + 1000);
    }
    auto deserialize(T &value, auto &source) const
        -> std::expected<void, typename std::remove_cvref_t<decltype(source)>::error_type> {
        if (auto r = source.template deserialize_integral<T>(value); !r) {
            return r;
        }
        value -= 1000;
        return {};
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
- `deserialize` 接收 `T &` 和 source 引用，返回 `std::expected<void, typename Source::error_type>`，并原地更新该值
- `deserialize` 内部调用 source 的值写入方法（如 `deserialize_integral`）获取原始值，并把错误原样返回

### 使用示例

```cpp
// 序列化
ustr text = serialize<TextTarget>(my_object);

// 反序列化（构造新值，返回 expected<T, error_type>）
auto result = deserialize<MyStruct, TextSource>(text);
if (result.has_value()) {
    // 使用 *result
}

// 反序列化（写入已有变量，返回 expected<void, error_type>）
MyStruct obj;
auto ok = deserialize<MyStruct, TextSource>(text, obj);

// 反序列化（直接操作 source）
TextSource src;
src.provide_source(text);
auto ok = deserialize(src, obj);
```
