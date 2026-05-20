#pragma once

#include <meta>
#include <type_traits>

namespace jungle::meta {

template<typename Anno>
consteval bool has_annotation(const std::meta::info type, const Anno) {
    if (std::meta::annotations_of_with_type(type, ^^Anno).size()) {
        return true;
    } else {
        return false;
    }
}

};  // namespace jungle::meta
