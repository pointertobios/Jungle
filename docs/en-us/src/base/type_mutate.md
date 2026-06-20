# type_mutate — Runtime Type-Mutable Base Class

`jungle::util::type_mutate<T>` is a CRTP base class that provides compile-time-safe runtime type querying and downcasting for derived classes. It does not depend on RTTI, instead using `type_id` for precise type identity comparison.

## Design Intent

In scenarios like ECS, `Component<>` and `Manager<>` need to flow through the system as untyped base class references, while also needing to be safely recovered to concrete derived types at runtime. `type_mutate` solves this by storing a `type_id` in each instance and providing cast methods with compile-time constraints.

## Core Mechanism

Every derived class must define a static member template `static_mutatable`, declaring which types are valid variants of that derived class:

```cpp
template<>
class Component<> : public util::type_mutate<Component<>> {
public:
    template<typename C>
    static constexpr bool static_mutatable = ComponentImpl<C>;
    // ...
};
```

`type_mutate` internally references this constraint via requires clauses:

```cpp
template<typename U>
    requires static_mutatable<U>
constexpr bool is() const { ... }
```

This means the compiler rejects queries against invalid types at compile time — if you try `base.is<WrongType>()`, the compiler will report an error rather than silently returning `false` at runtime.

## Construction

```cpp
protected:
    constexpr type_mutate(type_id type)
            : m_type{type} {}
```

The constructor is `protected`, callable only by derived classes. Derived classes must pass the correct `type_id` during construction (typically `type_id::of<DerivedType>()`).

## API

| Method                           | Description                                                                                          |
| -------------------------------- | ---------------------------------------------------------------------------------------------------- |
| `is<U>() const -> bool`          | Whether the current instance is of type `U`. Compile-time requires `U` satisfies `static_mutatable`  |
| `is(type_id) const -> bool`      | Whether the current instance's type ID matches the given `type_id` (no compile-time type constraint) |
| `as<U>() -> U &`                 | Downcast to `U&`. `pre(is<U>())` contract guarantee; checked in Debug mode                           |
| `as<U>() const -> const U &`     | const overload                                                                                       |
| `try_as<U>() -> U *`             | Safe downcast; returns `nullptr` on type mismatch                                                    |
| `try_as<U>() const -> const U *` | const overload                                                                                       |
| `type() const -> type_id`        | Returns the type ID stored in the current instance                                                   |

### is

```cpp
// Compile-time type-checked version — U must satisfy static_mutatable
template<typename U>
    requires static_mutatable<U>
constexpr bool is() const;

// Unconstrained version — accepts any type_id, suitable for data-driven type comparison
constexpr bool is(type_id type) const;
```

The former is used for type-safe compile-time dispatch; the latter is for scenarios requiring runtime `type_id` comparison (e.g., reading type identifiers from configuration or network).

### as

```cpp
template<typename U>
    requires static_mutatable<U>
constexpr U &as() pre(is<U>());
```

`as<U>()` performs a downcast. The `pre(is<U>())` contract requires the caller to guarantee type matching in Debug mode; Release mode uses a quick check. A type mismatch triggers `panic()`.

Typical usage:

```cpp
void process(Component<> &base) {
    if (base.is<HealthComponent>()) {
        auto &health = base.as<HealthComponent>();
        health.hp -= 10;
    }
}
```

### try_as

```cpp
template<typename U>
    requires static_mutatable<U>
constexpr U *try_as();
```

`try_as<U>()` is the safe, non-panicking version — returns `nullptr` on type mismatch. Suitable for cases where type matching cannot be guaranteed in advance or contract checks are undesirable:

```cpp
void maybe_process(Component<> &base) {
    if (auto *health = base.try_as<HealthComponent>()) {
        health->hp -= 10;
    }
    // or:
    // auto *health = base.try_as<HealthComponent>();
    // if (health) { ... }
}
```

The const overloads behave identically, returning `const U *` / `const U &`.

### type

```cpp
type_id type() const;
```

Directly returns the stored type ID, primarily used for logging, debugging, or type routing.

## Relationship with Component<> and Manager<>

`type_mutate` has two primary consumers in Jungle ECS:

- `Component<>`: all concrete Components inherit from this; `static_mutatable` is constrained to types satisfying the `ComponentImpl` concept
- `Manager<>`: all concrete Managers inherit from this; `static_mutatable` is constrained to types satisfying the `ManagerImpl` concept

This allows the system to operate uniformly on `Component<> &` or `Manager<> &` across different types, and safely recover concrete types when needed.

## Type Safety Tiers

| Tier         | Mechanism                     | Failure Behavior           |
| ------------ | ----------------------------- | -------------------------- |
| Compile-time | `static_mutatable` constraint | Compilation error          |
| Debug        | `pre(is<U>())` contract       | `panic()` aborts process   |
| Release      | `pre(is<U>())` quick check    | `panic()` aborts process   |
| Runtime-safe | `try_as<U>()` returns nullptr | Caller checks null pointer |
