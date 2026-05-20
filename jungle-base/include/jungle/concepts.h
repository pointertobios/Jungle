#pragma once

#include <concepts>

namespace jungle {

class ustr;

};

namespace jungle::concepts {

template<typename T>
concept DebugType = requires(T t) {
    { t.debug() } -> std::same_as<ustr>;
};

};  // namespace jungle::concepts
