#pragma once

#include <array>
#include <ranges>

#include "jungle/types/uchar.h"

namespace jungle::util {

/**
 * @brief 小端序 Base64 编码惰性视图
 * @details 产生一个 uchar 序列
 */
struct base64_encoder_view : public std::ranges::view_interface<base64_encoder_view> {
    static constexpr usize ENCODED_LENGTH = 12;

    struct iterator {
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = uchar;
        using difference_type = isize;

        iterator() noexcept = default;
        iterator(const base64_encoder_view *view, usize index) noexcept
                : m_view{view}
                , m_index{index} {}

        uchar operator*() const noexcept;

        iterator &operator++() noexcept;
        iterator operator++(int) noexcept;

        bool operator==(const iterator &other) const noexcept = default;

    private:
        const base64_encoder_view *m_view{nullptr};
        usize m_index{0};
    };

    base64_encoder_view(u64 value) noexcept
            : m_value{value} {}

    iterator begin() const noexcept;
    iterator end() const noexcept;

    usize size() const noexcept;

private:
    uchar at(usize index) const noexcept;

    u64 m_value;
};

};  // namespace jungle::util
