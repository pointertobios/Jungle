#pragma once

#include <meta>
#include <type_traits>
#include <vector>

#include "jungle/types/int.h"

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
 * @brief 判断实体是否含有指定模板注解的实例
 *
 * @tparam Instance 被判断的实体
 * @param temp_anno 目标模板注解
 * @return bool
 */
template<std::meta::info Instance>
consteval bool has_template_annotation(const std::meta::info temp_anno) {
    template for (constexpr auto anno : std::define_static_array(std::meta::annotations_of(Instance))) {
        constexpr auto anno_type = std::meta::remove_const(std::meta::type_of(anno));
        if constexpr (std::meta::has_template_arguments(anno_type)) {
            if (std::meta::template_of(anno_type) == std::meta::type_of(temp_anno)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 获取实体上指定模板注解的第 N 个实例
 *
 * @tparam Instance 被查询的实体
 * @param nth 第 N 个实例
 * @param temp_anno 目标模板注解
 * @return std::meta::info 注解实例的反射信息，如果未找到则返回 ^^void
 */
template<std::meta::info Instance>
consteval std::meta::info nth_template_annotation_of(usize nth, const std::meta::info temp_anno) {
    template for (constexpr auto anno : std::define_static_array(std::meta::annotations_of(Instance))) {
        constexpr auto anno_type = std::meta::remove_const(std::meta::type_of(anno));
        if constexpr (std::meta::has_template_arguments(anno_type)) {
            if (std::meta::template_of(anno_type) == std::meta::type_of(temp_anno)) {
                if (nth == 0) {
                    return anno;
                } else {
                    nth -= 1;
                }
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
