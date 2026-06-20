# Serde Serialization/Deserialization Framework

Jungle's `serde` is a C++26 reflection-based serialization/deserialization framework. It does not rely on macros or external code generation — all type information is obtained at compile time through reflection.

## Interfaces, Behavior, and Constraints

### Core Concepts

The framework decouples "how data is represented" from "what the data is":

- **Target**: Receives type-safe data write calls and produces some output format
- **Source**: Reads from some input format and writes into existing variables
- **Customizer**: Transforms the value of a specific field during serialization/deserialization

### Type Dispatch Order

The serialization and deserialization free functions perform compile-time dispatch on types in the following order:

1. `bool`
2. `std::integral`
3. `std::floating_point`
4. `enum`
5. `std::optional<T>`
6. `std::ranges::range` (container/range types)
7. `class` / `struct`

If a type does not match any of the above categories, a compile-time `static_assert` error is raised.

---

### Serialization

#### Free Functions

```cpp
// Write into an existing target
template<SerializeTargetImpl Target, typename T>
void serialize(const T &value, Target &target);

// Create a target and return the result
template<SerializeTargetImpl Target, typename T>
typename Target::target_type serialize(T &&value);
```

`serialize(value, target)` writes `value` into `target`. The convenience overload `serialize<Target>(value)` default-constructs `Target`, calls the above function, and returns the final product via `target.deliver_result()`.

#### SerializeTarget Base Class

All Targets must inherit from `SerializeTarget<Target>` (CRTP) and satisfy the `SerializeTargetImpl` concept:

- Define `target_type` (the type of the final product)
- Implement `deliver_result() -> target_type`
- Implement `spawn_subtarget() -> Target` (creates a child target for nested structures)
- Implement `serialize_*` methods for each type (called by the base class via CRTP)

Template methods provided by the `SerializeTarget` base class and their requires constraints:

| Base Class Method                         | Called Derived Class Methods                                                                                                                                                  |
| ----------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `serialize_bool(const bool &)`            | `serialize_bool(const bool &) -> void`                                                                                                                                        |
| `serialize_integral(const I &)`           | `serialize_integral(const I &) -> void`                                                                                                                                       |
| `serialize_floating_point(const F &)`     | `serialize_floating_point(const F &) -> void`                                                                                                                                 |
| `serialize_enum(const T &)`               | `serialize_enum(const T &) -> void`                                                                                                                                           |
| `serialize_optional(const optional<T> &)` | `serialize_optional_nonnull() -> void`, `serialize_optional_nullopt() -> void`                                                                                                |
| `serialize_range(const R &)`              | `serialize_range_head() -> void`, `serialize_range_element_end() -> void`, `serialize_range_tail(usize) -> void`                                                              |
| `serialize_class_object(const T &)`       | `serialize_class_head(string_view) -> void`, `serialize_class_field(string_view) -> void`, `serialize_class_field_end() -> void`, `serialize_class_tail(string_view) -> void` |

#### Class Serialization Flow

For class types, the framework uses reflection to iterate over all non-static data members:

- Without `[[=customized]]` annotation: all non-static data members are serialized
- With `[[=customized]]` annotation: only members marked with `[[=field]]` participate
- For members annotated with `[[=customize<C>]]`, the customizer `C`'s `serialize` method takes over the field's output

Reflection uses `std::meta::access_context::unchecked()` to access private members.

---

### Deserialization

#### Free Functions

```cpp
// Construct a new value from payload (T must be default_constructible)
// Returns optional containing the value on success, nullopt on failure
template<typename T, DeserializeSourceImpl Source>
  requires std::is_default_constructible_v<T>
std::optional<T> deserialize(const typename Source::source_type &source_payload);

// Write from payload into an existing variable, returns success status
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] bool deserialize(const typename Source::source_type &source_payload, T &value);

// Operate directly on a source, writing into an existing variable, returns success status
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] bool deserialize(Source &source, T &value);
```

All deserialization operations indicate success/failure via `bool` return values or `std::optional`. The caller is responsible for checking the return value.

#### DeserializeSource Base Class

All Sources must inherit from `DeserializeSource<Source>` (CRTP) and satisfy the `DeserializeSourceImpl` concept:

- Define `source_type` (the raw input type)
- Implement `provide_source(const source_type &) -> void`
- Implement `spawn_subsource() -> Source`
- Implement `deserialize_*` methods for each type; all methods return `bool`

Template methods provided by the `DeserializeSource` base class and their requires constraints:

| Base Class Method                           | Called Derived Class Methods                                                                                                                                 |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `deserialize_bool(bool &) -> bool`          | `deserialize_bool(bool &) -> bool`                                                                                                                           |
| `deserialize_integral(I &) -> bool`         | `deserialize_integral(I &) -> bool`                                                                                                                          |
| `deserialize_floating_point(F &) -> bool`   | `deserialize_floating_point(F &) -> bool`                                                                                                                    |
| `deserialize_enum(T &) -> bool`             | `deserialize_enum(T &) -> bool`                                                                                                                              |
| `deserialize_optional(OptionalT &) -> bool` | `deserialize_optional_nonnull() -> bool`, `deserialize_optional_nullopt() -> bool`                                                                           |
| `deserialize_range(R &) -> bool`            | `deserialize_range_head() -> bool`, `deserialize_range_has_element() -> bool`, `deserialize_range_element_end() -> bool`, `deserialize_range_tail() -> bool` |
| `deserialize_class_object(T &) -> bool`     | `deserialize_class_head() -> bool`, `deserialize_class_field() -> bool`, `deserialize_class_field_end() -> bool`, `deserialize_class_tail() -> bool`         |

> Unlike serialization, all deserialization structural methods (`head`, `field`, `element_end`, etc.) return `bool` rather than `void`. When an illegal format is encountered, they can return `false` to allow the upper layer to roll back or report an error, rather than terminating the process via panic.

#### Range Deserialization Flow

```
if (!deserialize_range_head()) return false;         → abort on failure
while deserialize_range_has_element():
    spawn_subsource() → create child source
    deserialize(subsource, elem) → recursively deserialize element (return value ignored; correctness guaranteed by parent source's structural methods)
    if (!deserialize_range_element_end()) return false;
if (!deserialize_range_tail()) return false;
```

#### Class Deserialization Flow

Symmetric with serialization, but `deserialize_class_head()` and `deserialize_class_field()` only return `bool` (success/failure), not the parsed type name/field name — these strings serve only as validation in deserialization, with failure handling unified by the base class control flow.

Customizer invocation:

```cpp
value.[:m:] = customizer_instance.deserialize(value.[:m:], subsource);
```

The customizer receives a member reference and a child source, and is responsible for reading and returning the transformed value.

---

### Annotations

| Annotation          | Purpose                                                                                             |
| ------------------- | --------------------------------------------------------------------------------------------------- |
| `[[=customized]]`   | Marks a class for customized serialization — only `[[=field]]` members participate                  |
| `[[=field]]`        | Marks a member to participate in serialization (only meaningful in `[[=customized]]` classes)       |
| `[[=customize<C>]]` | Specifies a field-level customizer `C`; the field's serialization/deserialization is handled by `C` |

Annotations may be combined:

```cpp
struct [[= customized]] S {
  [[= field]] int normal = 1;
  [[= field]] [[= customize<PlusThousand>]] int boosted = 100;
  int skipped = 999;   // no [[=field]], excluded from serialization
};
```

---

### Customizer Concept

```cpp
template<template<typename> typename Custr>
concept Customizer =
  std::is_default_constructible_v<Custr<int>> &&
  requires(Custr<int> c, int value, detail::TraitTargetSource &target) {
    { c.serialize(value, target) }      -> std::same_as<void>;
    { c.deserialize(value, target) }    -> std::same_as<int>;
  };
```

A customizer is a **single-parameter template**: `template<typename T> struct MyCustomizer { ... }`. The framework instantiates `MyCustomizer<FieldType>` for each field that uses the customizer.

- `serialize(const T &value, auto &target)`: writes `value` to `target` after transformation
- `deserialize(T &value, auto &source)`: reads the raw value from `source` into `value`, returns the transformed value

The customizer's `deserialize` return type should match the field type.

---

## Implementing Source/Target and Customizer

### Implementing TextTarget

The following is a Target implementation that serializes data into a human-readable text format (found in the unit test directory; for reference only):

```cpp
class TextTarget : public SerializeTarget<TextTarget> {
public:
    using target_type = ustr;

    TextTarget() = default;
    target_type deliver_result() { return std::move(m_result); }

    // spawn_subtarget lets nested structures share the same output buffer
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

Key design points:
- `spawn_subtarget()` shares the underlying buffer by reference; child target output is appended to the same string
- Uses `ustr::format` to format numerics, `debug()` to format enums
- The output format of each `serialize_*` method must be consistent with the corresponding `TextSource` `deserialize_*` parsing logic

### Implementing TextSource

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

    bool deserialize_bool(bool &value) { /* parse "true"/"false", write to value, return success */ }
    template<std::integral I>
    bool deserialize_integral(I &value) { /* from_chars parse, return success */ }
    template<std::floating_point F>
    bool deserialize_floating_point(F &value) { /* from_chars parse, return success */ }
    template<concepts::is_enum E>
    bool deserialize_enum(E &value) { /* match enum item by name, return success */ }

    bool deserialize_optional_nonnull() { /* consume "optional##", return true if not "nullopt" */ }
    bool deserialize_optional_nullopt() { /* consume "nullopt", return success */ }
    bool deserialize_range_head()        { /* consume "[" */ }
    bool deserialize_range_has_element() { /* check if next char is "]" */ }
    bool deserialize_range_element_end() { /* consume "," */ }
    bool deserialize_range_tail()        { /* consume "]" */ }
    bool deserialize_class_head()        { /* consume "TypeName{" */ }
    bool deserialize_class_field()       { /* consume "fieldName:" */ }
    bool deserialize_class_field_end()   { /* consume "," */ }
    bool deserialize_class_tail()        { /* consume "}" */ }

private:
    TextSource(std::string_view source, usize *cursor)
        : m_source{source}, m_cursor{cursor} {}
    std::string_view m_source;
    usize m_owned_cursor{0};
    usize *m_cursor{&m_owned_cursor};
};

static_assert(DeserializeSourceImpl<TextSource>);
```

Key design points:
- All methods return `bool`: return `true` on successful parse, `false` on format mismatch (no panic)
- `spawn_subsource()` shares a cursor: child source reads automatically advance the parent source's position
- `deserialize_optional_nonnull()` consumes the `"optional##"` prefix then only peeks (does not consume) `"nullopt"`; if it is indeed nullopt, returns `false` and `deserialize_optional_nullopt()` consumes it
- `deserialize_class_head()` and `deserialize_class_field()` only return `bool`, not the parsed type name/field name — these values are unused on the deserialization path

### Implementing a Customizer

The following customizer adds 1000 to the value during serialization and subtracts 1000 during deserialization:

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

Usage:

```cpp
struct S {
    [[= customize<PlusThousand>]] int score = 42;
};
```

A customizer must satisfy:
- It is a single-parameter template (`template<typename> struct`)
- `serialize` receives `const T &` and a target reference, returns `void`
- `deserialize` receives `T &` and a source reference, returns `T` (the transformed value)
- `deserialize` internally calls the source's value-writing method (e.g., `deserialize_integral`) to obtain the raw value

### Usage Examples

```cpp
// Serialization
ustr text = serialize<TextTarget>(my_object);

// Deserialization (construct new value, returns optional)
auto result = deserialize<MyStruct, TextSource>(text);
if (result.has_value()) {
    // use *result
}

// Deserialization (write into existing variable, returns bool)
MyStruct obj;
bool ok = deserialize<MyStruct, TextSource>(text, obj);

// Deserialization (operate directly on source)
TextSource src;
src.provide_source(text);
bool ok = deserialize(src, obj);
```
