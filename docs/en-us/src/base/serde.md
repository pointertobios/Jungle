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
// Returns the value on success, unexpected(Source::error_type) on failure
template<typename T, DeserializeSourceImpl Source>
  requires std::is_default_constructible_v<T>
std::expected<T, typename Source::error_type>
deserialize(const typename Source::source_type &source_payload);

// Write from payload into an existing variable
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] std::expected<void, typename Source::error_type>
deserialize(const typename Source::source_type &source_payload, T &value);

// Operate directly on a source, writing into an existing variable
template<typename T, DeserializeSourceImpl Source>
[[nodiscard]] std::expected<void, typename Source::error_type>
deserialize(Source &source, T &value);
```

All deserialization operations indicate success/failure via `std::expected`: overloads that write into an existing variable return `expected<void, Source::error_type>`, and the overload that constructs a new value returns `expected<T, Source::error_type>`. Errors propagate from `deserialize_*` up to these free functions. The caller is responsible for checking the return value.

#### DeserializeSource Base Class

All Sources must inherit from `DeserializeSource<Source>` (CRTP) and satisfy the `DeserializeSourceImpl` concept:

- Define `source_type` (the raw input type)
- Define `error_type` (the error type reported on deserialization failure)
- Implement `provide_source(const source_type &) -> void`
- Implement `spawn_subsource() -> Source`
- Implement `deserialize_*` methods for each type; all methods return `std::expected<void, error_type>`

Template methods provided by the `DeserializeSource` base class and their requires constraints (`E` stands for `Source::error_type`):

| Base Class Method                                              | Called Derived Class Methods                                                                                                                                                                                                         |
| -------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `deserialize_bool(bool &) -> expected<void, E>`                | `deserialize_bool(bool &) -> expected<void, E>`                                                                                                                                                                                      |
| `deserialize_integral(I &) -> expected<void, E>`               | `deserialize_integral(I &) -> expected<void, E>`                                                                                                                                                                                     |
| `deserialize_floating_point(F &) -> expected<void, E>`         | `deserialize_floating_point(F &) -> expected<void, E>`                                                                                                                                                                               |
| `deserialize_enum(T &) -> expected<void, E>`                   | `deserialize_enum(T &) -> expected<void, E>`                                                                                                                                                                                         |
| `deserialize_optional(OptionalT &) -> expected<void, E>`       | `deserialize_optional_nonnull() -> expected<void, E>`, `deserialize_optional_nullopt() -> expected<void, E>`                                                                                                                         |
| `deserialize_range(R &) -> expected<void, E>`                  | `deserialize_range_head() -> expected<void, E>`, `deserialize_range_has_element() -> expected<void, E>`, `deserialize_range_element_end() -> expected<void, E>`, `deserialize_range_tail() -> expected<void, E>`                     |
| `deserialize_class_object(T &) -> expected<void, E>`           | `deserialize_class_head() -> expected<void, E>`, `deserialize_class_field() -> expected<void, E>`, `deserialize_class_field_end() -> expected<void, E>`, `deserialize_class_tail() -> expected<void, E>`                             |

> Unlike serialization, all deserialization structural methods (`head`, `field`, `element_end`, etc.) return `expected<void, E>` rather than `void`. When an illegal format is encountered, they can return `unexpected` to allow the upper layer to roll back or report an error, rather than terminating the process via panic.
>
> `deserialize_optional_nonnull` / `deserialize_optional_nullopt` and `deserialize_range_has_element` use success to mean "this case matched / more elements remain", and `unexpected` to mean "this case did not match / no more elements". The base class propagates the latter error when both optional paths fail; for ranges, a failed `has_element` ends the loop and parsing continues at the tail.

#### Range Deserialization Flow

```
if (auto r = deserialize_range_head(); !r) return r;   → abort and propagate on failure
while deserialize_range_has_element() succeeds:
    spawn_subsource() → create child source
    if (auto r = deserialize(subsource, elem); !r) return r;  → recursively deserialize element
    if (auto r = deserialize_range_element_end(); !r) return r;
if (auto r = deserialize_range_tail(); !r) return r;
```

#### Class Deserialization Flow

Symmetric with serialization, but `deserialize_class_head()` and `deserialize_class_field()` only return `expected<void, E>` (success/failure), not the parsed type name/field name — these strings serve only as validation in deserialization, with failure handling unified by the base class control flow.

The customizer receives a member reference and a child source, writes into the member in place, and returns `expected`; the base class checks that result and propagates it:

```cpp
if (auto r = customizer_instance.deserialize(value.[:m:], subsource); !r) {
    return r;
}
```

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
    { c.deserialize(value, target) }
      -> std::same_as<std::expected<void, typename detail::TraitTargetSource::error_type>>;
  };
```

A customizer is a **single-parameter template**: `template<typename T> struct MyCustomizer { ... }`. The framework instantiates `MyCustomizer<FieldType>` for each field that uses the customizer.

- `serialize(const T &value, auto &target)`: writes `value` to `target` after transformation, returns `void`
- `deserialize(T &value, auto &source)`: reads the raw value from `source` into `value` and writes the transformed value in place, returning `std::expected<void, typename Source::error_type>`

`deserialize` must propagate errors from the `deserialize_*` calls it delegates to; the framework checks that return value and continues propagating it.

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
    enum class error_type { mismatch };

    TextSource() = default;
    void provide_source(const source_type &source) {
        m_source = source.view();
        *m_cursor = 0;
    }
    TextSource spawn_subsource() { return TextSource{m_source, m_cursor}; }

    std::expected<void, error_type> deserialize_bool(bool &value) { /* parse "true"/"false" */ }
    template<std::integral I>
    std::expected<void, error_type> deserialize_integral(I &value) { /* from_chars parse */ }
    template<std::floating_point F>
    std::expected<void, error_type> deserialize_floating_point(F &value) { /* from_chars parse */ }
    template<concepts::is_enum E>
    std::expected<void, error_type> deserialize_enum(E &value) { /* match enum item by name */ }

    std::expected<void, error_type> deserialize_optional_nonnull() { /* consume "optional##"; succeed if not "nullopt" */ }
    std::expected<void, error_type> deserialize_optional_nullopt() { /* consume "nullopt" */ }
    std::expected<void, error_type> deserialize_range_head()        { /* consume "[" */ }
    std::expected<void, error_type> deserialize_range_has_element() { /* check if next char is "]" */ }
    std::expected<void, error_type> deserialize_range_element_end() { /* consume "," */ }
    std::expected<void, error_type> deserialize_range_tail()        { /* consume "]" */ }
    std::expected<void, error_type> deserialize_class_head()        { /* consume "TypeName{" */ }
    std::expected<void, error_type> deserialize_class_field()       { /* consume "fieldName:" */ }
    std::expected<void, error_type> deserialize_class_field_end()   { /* consume "," */ }
    std::expected<void, error_type> deserialize_class_tail()        { /* consume "}" */ }

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
- All methods return `std::expected<void, error_type>`: return `{}` on successful parse, `unexpected` on format mismatch (no panic)
- `spawn_subsource()` shares a cursor: child source reads automatically advance the parent source's position
- `deserialize_optional_nonnull()` consumes the `"optional##"` prefix then only peeks (does not consume) `"nullopt"`; if it is indeed nullopt, returns `unexpected` and `deserialize_optional_nullopt()` consumes it
- `deserialize_class_head()` and `deserialize_class_field()` only return `expected<void, error_type>`, not the parsed type name/field name — these values are unused on the deserialization path

### Implementing a Customizer

The following customizer adds 1000 to the value during serialization and subtracts 1000 during deserialization:

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

Usage:

```cpp
struct S {
    [[= customize<PlusThousand>]] int score = 42;
};
```

A customizer must satisfy:
- It is a single-parameter template (`template<typename> struct`)
- `serialize` receives `const T &` and a target reference, returns `void`
- `deserialize` receives `T &` and a source reference, returns `std::expected<void, typename Source::error_type>`, and updates the value in place
- `deserialize` internally calls the source's value-writing method (e.g., `deserialize_integral`) to obtain the raw value, and propagates that error unchanged

### Usage Examples

```cpp
// Serialization
ustr text = serialize<TextTarget>(my_object);

// Deserialization (construct new value, returns expected<T, error_type>)
auto result = deserialize<MyStruct, TextSource>(text);
if (result.has_value()) {
    // use *result
}

// Deserialization (write into existing variable, returns expected<void, error_type>)
MyStruct obj;
auto ok = deserialize<MyStruct, TextSource>(text, obj);

// Deserialization (operate directly on source)
TextSource src;
src.provide_source(text);
auto ok = deserialize(src, obj);
```
