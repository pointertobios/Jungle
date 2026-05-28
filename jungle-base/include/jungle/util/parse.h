#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <type_traits>

#include "jungle/types/uchar.h"

namespace jungle::util {

/**
 * @brief 小端序 Base64 编码惰性视图
 * @details 产生一个 uchar 序列
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
