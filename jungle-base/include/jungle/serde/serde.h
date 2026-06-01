#pragma once

#include <concepts>
#include <type_traits>

/**
 * @brief 序列化反序列化框架
 *
 * 本框架提供一套统一的工具集合以桥接任意目标数据格式的序列化和反序列化。
 *
 * - `serialize` 函数集合：将 C++ 对象序列化为指定目标格式，仅处理非静态数据成员
 * - `SerializeTarget`：序列化目标 CRTP 基类，向 serialize 函数提供序列化接口
 * - `deserialize` 函数集合：从指定源格式反序列化为 C++ 对象，仅处理非静态数据成员
 * - `DeserializeSource`：反序列化源 CRTP 基类，向 deserialize 函数提供反序列化接口
 * - `customized` 注解：标记类为定制序列化，仅序列化带有 `field` 注解的成员
 * - `customize<C>` 注解：标记成员使用自定义序列化器 `C`，该序列化器必须满足 `Customizer` 概念
 *
 * > serialize 及 deserialize 相关文档见 `serialize.h` 和 `deserialize.h`
 *
 * ## Customizer
 *
 * 概念 `Customizer<C>` 定义了一个类型 `C` 作为自定义序列化器的要求。满足该概念的模板类型必须：
 *
 * - 对任意类型 `T` 都可默认构造 `C<T>` 实例
 * - 提供成员函数 `void serialize(const T &value, auto &target) const`，用于序列化 `T` 类型的值
 * - 提供成员函数 `T deserialize(T &value, auto &source) const`，用于反序列化 `T` 类型的值
 */

namespace jungle::serde {

namespace detail {

class TraitTargetSource {
public:
    using result_type = bool;

    bool deliver_result() { return false; }

    template<std::integral I>
    void serialize_integral(I) {}

    template<std::floating_point F>
    void serialize_floating_point(F) {}

    void serialize_bool(bool) {}

    template<concepts::is_enum E>
    void serialize_enum(E) {}

    void serialize_range_head() {}
    void serialize_range_element_end() {}
    void serialize_range_tail(usize) {}

    void serialize_class_head(std::string_view) {}
    void serialize_class_field(std::string_view) {}
    void serialize_class_field_end() {}
    void serialize_class_tail(std::string_view) {}

    void deserialize_bool(bool &) {}

    template<std::integral I>
    void deserialize_integral(I &) {}

    template<std::floating_point F>
    void deserialize_floating_point(F &) {}

    template<concepts::is_enum E>
    void deserialize_enum(E &) {}

    bool deserialize_optional_nonnull() { return {}; }

    void deserialize_range_head() {}
    bool deserialize_range_has_element() { return {}; }
    void deserialize_range_element_end() {}
    void deserialize_range_tail() {}

    std::string_view deserialize_class_head() { return {}; }
    std::string_view deserialize_class_field() { return {}; }
    void deserialize_class_field_end() {}
    void deserialize_class_tail() {}
};

};  // namespace detail

template<typename>
class SerializeTarget;

template<typename T>
concept SerializeTargetImpl =
    std::derived_from<T, SerializeTarget<T>> && !std::is_same_v<T, SerializeTarget<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::target_type;
           { t.deliver_result() } -> std::same_as<typename T::target_type>;
       };

template<typename>
class DeserializeSource;

template<typename T>
concept DeserializeSourceImpl =
    std::derived_from<T, DeserializeSource<T>> && !std::is_same_v<T, DeserializeSource<T>>
    && std::is_default_constructible_v<T> && requires(T t) {
           typename T::source_type;
           { t.provide_source(std::declval<const typename T::source_type &>()) } -> std::same_as<void>;
       };

template<template<typename> typename Custr>
concept Customizer = std::is_default_constructible_v<Custr<int>>
                     && requires(Custr<int> customizer, int value, detail::TraitTargetSource &target) {
                            { customizer.serialize(value, target) } -> std::same_as<void>;
                            { customizer.deserialize(value, target) } -> std::same_as<int>;
                        };

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
struct Customize {};

template<template<typename> typename Custr>
    requires(Customizer<Custr>)
inline constexpr Customize<Custr> customize;

inline constexpr struct {
} customized;

inline constexpr struct {
} field;

};  // namespace jungle::serde
