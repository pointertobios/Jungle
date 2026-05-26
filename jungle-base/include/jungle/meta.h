#pragma once

#include <meta>
#include <type_traits>
#include <vector>

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

template<typename Anno>
consteval std::vector<std::meta::info>
nonstatic_data_members_with_annotation(const std::meta::info type, const Anno anno) {
    std::vector<std::meta::info> result;
    template for (constexpr auto m : std::define_static_array(std::meta::members_of(type))) {
        if (std::meta::is_nonstatic_data_member(m) && has_annotation(m, anno)) {
            result.push_back(m);
        }
    }
    return result;
}

};  // namespace jungle::meta
