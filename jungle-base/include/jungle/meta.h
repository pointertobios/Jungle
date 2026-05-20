#pragma once

#include <meta>
#include <type_traits>

namespace jungle::meta {

/**
 * @brief 通过注解的类型判断类型是否有注解
 * @tparam Anno
 * @param type 被 Anno 值注解的类型反射信息
 * @param _: Anno 值
 * @return
 */
template<typename Anno>
consteval bool has_annotation(const std::meta::info type, const Anno _) {
    if (std::meta::annotations_of_with_type(type, ^^Anno).size()) {
        return true;
    } else {
        return false;
    }
}

};  // namespace jungle::meta
