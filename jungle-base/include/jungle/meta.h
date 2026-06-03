// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <meta>
#include <type_traits>
#include <vector>

#include "jungle/types/int.h"

namespace jungle::meta {

/**
 * @brief 閫氳繃娉ㄨВ鐨勭被鍨嬪垽鏂被鍨嬫槸鍚︽湁娉ㄨВ
 * @param type 琚?Anno 鍊兼敞瑙ｇ殑绫诲瀷鍙嶅皠淇℃伅
 * @param anno 娉ㄨВ
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
 * @brief 鍒ゆ柇瀹炰綋鏄惁鍚湁鎸囧畾妯℃澘娉ㄨВ鐨勫疄渚?
 *
 * @tparam Instance 琚垽鏂殑瀹炰綋
 * @tparam TemplateAnnotation 妯℃澘娉ㄨВ锛岃姹傛ā鏉挎敞瑙ｇ殑绫诲瀷鏈韩蹇呴』鏄拰娉ㄨВ鏈韩鐩稿悓鐨勬ā鏉?
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
 * @brief 鑾峰彇瀹炰綋涓婃寚瀹氭ā鏉挎敞瑙ｇ殑绗?N 涓ā鏉垮弬鏁?
 *
 * @tparam Instance 琚煡璇㈢殑瀹炰綋
 * @tparam TemplateAnnotation 妯℃澘娉ㄨВ
 * @param nth 妯℃澘娉ㄨВ绗?nth 涓ā鏉垮弬鏁?
 * @return std::meta::info 娉ㄨВ瀹炰緥鐨勫弽灏勪俊鎭?
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
 * @brief 鍒ゆ柇妯℃澘瀹炰緥鏄惁鏄寚瀹氭ā鏉跨殑涓€涓疄渚?
 *
 * @param specialization 妯℃澘瀹炰緥
 * @param template_info 妯℃澘淇℃伅
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
