#pragma once

#include <meta>
#include <type_traits>
#include <vector>

#include "jungle/types/int.h"

namespace jungle::meta {

/**
 * @brief 通过注解的类型判断类型是否有注解
 * @param type 被 Anno 值注解的类型反射信息
 * @param anno 注解
 * @return bool
 */
consteval bool has_annotation(const std::meta::info type, const auto anno) {
    if (std::meta::annotations_of_with_type(type, ^^decltype(anno)).size()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 判断实体是否含有指定模板注解的实例
 *
 * @tparam Instance 被判断的实体
 * @tparam TemplateAnnotation 模板注解
 * @return bool
 */
template<std::meta::info Instance, std::meta::info TemplateAnnotation>
consteval bool has_template_annotation() {
    template for (constexpr auto anno_obj : std::define_static_array(std::meta::annotations_of(Instance))) {
        constexpr auto anno = std::meta::constant_of(anno_obj);
        if constexpr (std::meta::has_template_arguments(std::meta::type_of(anno))) {
            constexpr auto targs_list =
                std::define_static_array(std::meta::template_arguments_of(std::meta::type_of(anno)));
            constexpr auto instanciated_anno = std::meta::substitute(TemplateAnnotation, targs_list);
            if (std::meta::type_of(anno) == std::meta::type_of(instanciated_anno)) {
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
 * @tparam TemplateAnnotation 模板注解
 * @param nth 第 N 个实例
 * @return std::meta::info 注解实例的反射信息，如果未找到则返回 ^^void
 */
template<std::meta::info Instance, std::meta::info TemplateAnnotation>
consteval std::meta::info nth_template_annotation_argument_of(usize nth) {
    template for (constexpr auto anno_obj : std::define_static_array(std::meta::annotations_of(Instance))) {
        constexpr auto anno = std::meta::constant_of(anno_obj);
        if constexpr (std::meta::has_template_arguments(std::meta::type_of(anno))) {
            constexpr auto targs_list =
                std::define_static_array(std::meta::template_arguments_of(std::meta::type_of(anno)));
            constexpr auto instanciated_anno = std::meta::substitute(TemplateAnnotation, targs_list);
            if (std::meta::type_of(anno) == std::meta::type_of(instanciated_anno)) {
                return targs_list[nth];
            }
        }
    }
    return ^^void;
}

consteval std::vector<std::meta::info>
nonstatic_data_members_with_annotation(const std::meta::info type, const auto anno) {
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
