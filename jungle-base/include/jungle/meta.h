#pragma once

#include <meta>
#include <type_traits>
#include <vector>

namespace jungle::meta {

/**
 * @brief 通过注解的类型判断类型是否有注解
 * @tparam Anno
 * @param type 被 Anno 值注解的类型反射信息
 * @param _ Anno 注解
 * @return bool
 */
template<typename Anno>
consteval bool has_annotation(const std::meta::info type, const Anno _) {
    if (std::meta::annotations_of_with_type(type, ^^Anno).size()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 判断是否含有指定模板注解的实例
 *
 * @param type 被判断的实体
 * @param temp_anno 目标模板注解
 * @return bool
 */
consteval bool has_template_annotation(const std::meta::info type, const std::meta::info temp_anno)
    pre(std::meta::is_variable_template(temp_anno)) {
    template for (constexpr auto anno : std::define_static_array(std::meta::annotations_of(type))) {
        if constexpr (std::meta::has_template_arguments(anno)) {
            if (std::meta::template_of(anno) == temp_anno) {
                return true;
            }
        }
    }
    return false;
}

consteval std::meta::info
first_template_annotation_of(const std::meta::info type, const std::meta::info temp_anno)
    pre(std::meta::is_variable_template(temp_anno)) {
    template for (constexpr auto anno : std::define_static_array(std::meta::annotations_of(type))) {
        if constexpr (std::meta::has_template_arguments(anno)) {
            if (std::meta::template_of(anno) == temp_anno) {
                return anno;
            }
        }
    }
    return ^^void;
}

template<typename Anno>
consteval std::vector<std::meta::info>
nonstatic_data_members_with_annotation(const std::meta::info type, const Anno anno) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    std::vector<std::meta::info> result;
    template for (constexpr auto m : std::define_static_array(std::meta::members_of(type, ctx))) {
        if (std::meta::is_nonstatic_data_member(m) && has_annotation(m, anno)) {
            result.push_back(m);
        }
    }
    return result;
}

};  // namespace jungle::meta
