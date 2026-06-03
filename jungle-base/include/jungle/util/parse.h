// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <type_traits>

#include "jungle/types/uchar.h"

namespace jungle::util {

/**
 * @brief 灏忕搴?Base64 缂栫爜鎯版€ц鍥?
 * @details 浜х敓涓€涓?uchar 搴忓垪
 */
struct base64_encoder_view : public std::ranges::view_interface<base64_encoder_view> {
    struct iterator {
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = uchar;
        using difference_type = isize;

        iterator() = default;
        iterator(const base64_encoder_view *view, usize index)
                : m_view{view}
                , m_index{index} {}

        uchar operator*() const;

        iterator &operator++();
        iterator operator++(int);

        bool operator==(const iterator &other) const = default;

    private:
        const base64_encoder_view *m_view{nullptr};
        usize m_index{0};
    };

    template<std::ranges::contiguous_range R>
        requires std::same_as<std::ranges::range_value_t<R>, u8>
    constexpr base64_encoder_view(R &range)
            : m_data(std::ranges::data(range))
            , m_n(static_cast<usize>(std::ranges::size(range))) {}

    iterator begin() const;
    iterator end() const;

    usize size() const;

private:
    uchar at(usize index) const;

    const u8 *m_data{nullptr};
    usize m_n{0};
};

};  // namespace jungle::util
