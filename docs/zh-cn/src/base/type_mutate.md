# type_mutate 运行时类型可变基类

`jungle::util::type_mutate<T>` 是一个 CRTP 基类，为派生类提供编译期安全的运行时类型查询与向下转型能力。它不依赖 RTTI，而是基于 `type_id` 做精确的类型标识比对。

## 设计意图

在 ECS 等场景中，`Component<>` 和 `Manager<>` 需要以无类型基类引用的方式在系统中流转，同时又要在运行时安全地恢复为具体派生类型。`type_mutate` 通过在每个实例中存储一个 `type_id` 并提供带编译期约束的转型方法，解决了这一需求。

## 核心机制

每个派生类必须定义一个静态成员模板 `static_mutatable`，声明哪些类型是该派生类的合法变体：

```cpp
template<>
class Component<> : public util::type_mutate<Component<>> {
public:
    template<typename C>
    static constexpr bool static_mutatable = ComponentImpl<C>;
    // ...
};
```

`type_mutate` 内部通过 requires 子句引用此约束：

```cpp
template<typename U>
    requires static_mutatable<U>
constexpr bool is() const { ... }
```

这意味着编译期即可拒绝对非法类型的查询——如果你试图 `base.is<WrongType>()`，编译器会直接报错，而非等到运行时返回 `false`。

## 构造

```cpp
protected:
    constexpr type_mutate(type_id type)
            : m_type{type} {}
```

构造函数为 `protected`，仅派生类可调用。派生类在构造时必须传入正确的 `type_id`（通常为 `type_id::of<DerivedType>()`）。

## API

| 方法                             | 说明                                                            |
| -------------------------------- | --------------------------------------------------------------- |
| `is<U>() const -> bool`          | 当前实例是否为 `U` 类型。编译期要求 `U` 满足 `static_mutatable` |
| `is(type_id) const -> bool`      | 当前实例的类型 ID 是否匹配指定 `type_id`（无编译期类型约束）    |
| `as<U>() -> U &`                 | 向下转型为 `U&`。`pre(is<U>())` 契约保证，Debug 模式检查        |
| `as<U>() const -> const U &`     | const 重载版                                                    |
| `try_as<U>() -> U *`             | 安全向下转型，类型不匹配返回 `nullptr`                          |
| `try_as<U>() const -> const U *` | const 重载版                                                    |
| `type() const -> type_id`        | 返回当前实例存储的类型 ID                                       |

### is

```cpp
// 编译期类型检查版——U 必须满足 static_mutatable
template<typename U>
    requires static_mutatable<U>
constexpr bool is() const;

// 无约束版——接受任意 type_id，适合数据驱动的类型比对
constexpr bool is(type_id type) const;
```

前者用于类型安全的编译期分派，后者用于需要运行时 `type_id` 比对的场景（如从配置或网络读取类型标识）。

### as

```cpp
template<typename U>
    requires static_mutatable<U>
constexpr U &as() pre(is<U>());
```

`as<U>()` 执行向下转型。`pre(is<U>())` 契约要求调用方在 Debug 模式下保证类型匹配，Release 模式下为快速检查。若类型不匹配则触发 `panic()`。

典型用法：

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

`try_as<U>()` 是安全的非 panic 版本——类型不匹配时返回 `nullptr`。适合无法预先保证类型匹配、也不想触发契约检查的场景：

```cpp
void maybe_process(Component<> &base) {
    if (auto *health = base.try_as<HealthComponent>()) {
        health->hp -= 10;
    }
    // 或：
    // auto *health = base.try_as<HealthComponent>();
    // if (health) { ... }
}
```

const 重载行为一致，返回 `const U *` / `const U &`。

### type

```cpp
type_id type() const;
```

直接返回存储的类型 ID，主要用于日志、调试或类型路由。

## 与 Component<> 和 Manager<> 的关系

`type_mutate` 在 Jungle ECS 中有两个主要使用者：

- `Component<>`：所有具体 Component 继承自此，`static_mutatable` 约束为满足 `ComponentImpl` concept 的类型
- `Manager<>`：所有具体 Manager 继承自此，`static_mutatable` 约束为满足 `ManagerImpl` concept 的类型

这使得系统可以用 `Component<> &` 或 `Manager<> &` 统一操作不同类型，并在需要具体类型时安全恢复。

## 类型安全层级

| 层级       | 机制                       | 失败行为             |
| ---------- | -------------------------- | -------------------- |
| 编译期     | `static_mutatable` 约束    | 编译错误             |
| Debug      | `pre(is<U>())` 契约        | `panic()` 中止进程   |
| Release    | `pre(is<U>())` 快速检查    | `panic()` 中止进程   |
| 运行时安全 | `try_as<U>()` 返回 nullptr | 调用方自行检查空指针 |
