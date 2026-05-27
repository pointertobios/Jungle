#pragma once

#include <concepts>
#include <type_traits>

namespace jungle {

class ustr;

};

namespace jungle::concepts {

template<typename T>
concept Debug = requires(T t) {
    { t.debug() } -> std::same_as<ustr>;
};

template<typename T>
concept is_void = std::is_void_v<T>;

template<typename T>
concept non_void = !std::is_void_v<T>;

template<typename T>
concept is_enum = std::is_enum_v<T>;

};  // namespace jungle::concepts
