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
 * @tparam TemplateAnnotation 模板注解，要求模板注解的类型本身必须是和注解本身相同的模板
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
 * @brief 获取实体上指定模板注解的第 N 个模板参数
 *
 * @tparam Instance 被查询的实体
 * @tparam TemplateAnnotation 模板注解
 * @param nth 模板注解第 nth 个模板参数
 * @return std::meta::info 注解实例的反射信息
 */
template<std::meta::info Instance, std::meta::info TemplateAnnotation>
consteval std::meta::info nth_template_annotation_argument_of(usize nth) {
    static_assert(
        has_template_annotation<Instance, TemplateAnnotation>(),
        "Instance should have the specified template annotation");
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
}

/**
 * @brief 判断模板实例是否是指定模板的一个实例
 *
 * @param specialization 模板实例
 * @param template_info 模板信息
 * @return bool
 */
template<std::meta::info Specialization, std::meta::info Template>
consteval bool is_specialization_of_template() {
    constexpr auto specialization = std::meta::dealias(Specialization);
    constexpr auto template_info = std::meta::dealias(Template);
    if (!std::meta::has_template_arguments(specialization)) {
        return false;
    }
    return std::meta::template_of(specialization) == template_info;
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
