// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "jungle/panic.h"
#include "jungle/types/concepts.h"
#include "jungle/types/int.h"
#include "jungle/types/raw_storage.h"
#include "jungle/types/types.h"
#include "jungle/util/murmur.h"

namespace jungle {

template<typename K>
concept hash_key =
    std::is_default_constructible_v<K> && std::is_copy_assignable_v<K> && std::is_copy_constructible_v<K>
    && std::is_destructible_v<K> && std::equality_comparable<K> && std::copyable<K> && requires(K k) {
           { std::hash<K>{}(k) } -> std::convertible_to<usize>;
       };

namespace detail {

using concepts::non_void;

template<hash_key K, non_void V>
struct pair_ref {
private:
    const K &m_key;
    V &m_value;

public:
    pair_ref(const K &key, V &value)
            : m_key{key}
            , m_value{value} {}

    const K &key() const { return m_key; }

    std::tuple_element_t<1, pair_ref<K, V>> value() { return m_value; }

    const std::tuple_element_t<1, pair_ref<K, V>> value() const { return m_value; }
};

template<hash_key K, non_void V>
    requires(!std::is_void_v<V>)
struct pair_ref_const {
private:
    const K &m_key;
    const V &m_value;

public:
    pair_ref_const(const K &key, const V &value)
            : m_key{key}
            , m_value{value} {}

    const K &key() const { return m_key; }

    std::tuple_element_t<1, pair_ref_const<K, V>> value() const { return m_value; }
};

template<jungle::usize I, jungle::hash_key K, jungle::concepts::non_void V>
std::tuple_element_t<I, jungle::detail::pair_ref<K, V>> get(const jungle::detail::pair_ref<K, V> &p) {
    if constexpr (I == 0) {
        return p.key();
    } else {
        return p.value();
    }
}

template<jungle::usize I, jungle::hash_key K, jungle::concepts::non_void V>
std::tuple_element_t<I, jungle::detail::pair_ref_const<K, V>>
get(const jungle::detail::pair_ref_const<K, V> &p) {
    if constexpr (I == 0) {
        return p.key();
    } else {
        return p.value();
    }
}

};  // namespace detail

};  // namespace jungle

template<
    jungle::hash_key K, jungle::concepts::non_void V, template<class...> typename TQual,
    template<class...> typename UQual>
struct std::basic_common_reference<
    jungle::detail::pair_ref<K, V>, jungle::detail::pair_ref<K, V>, TQual, UQual> {
    using type = const jungle::detail::pair_ref<K, V> &;
};

template<
    jungle::hash_key K, jungle::concepts::non_void V, template<class...> typename TQual,
    template<class...> typename UQual>
struct std::basic_common_reference<
    jungle::detail::pair_ref_const<K, V>, jungle::detail::pair_ref_const<K, V>, TQual, UQual> {
    using type = const jungle::detail::pair_ref_const<K, V> &;
};

namespace jungle {

template<hash_key K, typename V>
class hash_map {
    struct slot {
        enum class state {
            empty,
            filled,
            tombstone,
        } st;
        K key;
        [[no_unique_address]] raw_storage<V> value;
    };

    constexpr static usize default_slots_size = 64;

    constexpr static usize load_factor_numerator = 3;
    constexpr static usize load_factor_denominator = 4;

    enum class try_insert_result {
        succeeded,
        rehash_needed,
        duplicated,
    };

public:
    class iterator {
        friend class hash_map;

        using mut_value_type = fuck_void_or_else<V, detail::pair_ref<K, fuck_void<V>>, K>;
        using const_value_type = fuck_void_or_else<V, detail::pair_ref_const<K, fuck_void<V>>, K>;

    public:
        using difference_type = isize;
        using value_type = mut_value_type;
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;

        iterator() = default;

        ~iterator() {
            if (m_cache_valid) {
                m_cache.destroy();
            }
        }

        value_type &operator*() const pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            if (m_cache_valid) {
                m_cache.destroy();
            }
            auto &sl = m_map->m_slots.at(m_index.value());
            if constexpr (std::is_void_v<V>) {
                m_cache.emplace(sl.key);
            } else {
                m_cache.emplace(sl.key, *sl.value.get());
            }
            m_cache_valid = true;
            return *m_cache.get();
        }

        iterator &operator++() pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            m_index.value() += 1;
            m_counter += 1;

            m_cache_valid = false;
            next_filled();
            return *this;
        }

        iterator operator++(int) pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            iterator res = *this;
            (*this)++;
            return res;
        }

        iterator(const iterator &other)
                : m_map{other.m_map}
                , m_generation{other.m_generation}
                , m_counter{other.m_counter}
                , m_index{other.m_index} {}

        iterator &operator=(const iterator &other) {
            if (this == &other) {
                return *this;
            }
            if (m_cache_valid) {
                m_cache.destroy();
            }
            m_map = other.m_map;
            m_generation = other.m_generation;
            m_counter = other.m_counter;
            m_index = other.m_index;
            m_cache_valid = false;
            return *this;
        }

        iterator(iterator &&other)
                : m_map{other.m_map}
                , m_generation{other.m_generation}
                , m_counter{other.m_counter}
                , m_index{other.m_index} {
            other.m_index = std::nullopt;
            other.m_counter = 0;
            if (other.m_map) {
                other.m_generation = other.m_map->m_generation + 1;
                other.m_map = nullptr;
            } else {
                other.m_generation = other.m_generation + 1;
            }
        }

        iterator &operator=(iterator &&other) {
            if (this == &other) {
                return *this;
            }
            if (m_cache_valid) {
                m_cache.destroy();
            }
            m_map = other.m_map;
            m_generation = other.m_generation;
            m_counter = other.m_counter;
            m_index = other.m_index;
            m_cache_valid = false;
            other.m_index = std::nullopt;
            other.m_counter = 0;
            if (other.m_map) {
                other.m_generation = other.m_map->m_generation + 1;
                other.m_map = nullptr;
            } else {
                other.m_generation = other.m_generation + 1;
            }
            return *this;
        }

        bool operator==(const iterator &rhs) const
            pre(validative_check() && rhs.validative_check() && m_map == rhs.m_map) {
            if (!(validative_check() && rhs.validative_check() && m_map == rhs.m_map)) {
                panic("assertion failed: validative_check() && rhs.validative_check() && m_map == rhs.m_map");
            }  // TODO: workaround gcc contract bug for template function
            return m_index == rhs.m_index;
        }

        friend difference_type operator-(const iterator &lhs, const iterator &rhs)
            pre(lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map) {
            if (!(lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map)) {
                panic(
                    "assertion failed: lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map");
            }  // TODO: workaround gcc contract bug for template function
            return static_cast<difference_type>(lhs.m_counter) - static_cast<difference_type>(rhs.m_counter);
        }

    private:
        iterator(hash_map &map, usize generation, bool end = false)
                : m_map{&map}
                , m_generation{generation}
                , m_counter{end ? map.m_load : 0}
                , m_index{end ? std::optional<usize>{} : std::optional<usize>{0}} {
            if (!end) {
                next_filled();
            }
        }

        void next_filled() {
            if (!m_index.has_value()) {
                return;
            }
            if (m_index.value() == m_map->m_slots.size()) {
                m_index = std::nullopt;
                return;
            }
            while (m_index.value() < m_map->m_slots.size()
                   && m_map->m_slots.at(m_index.value()).st != slot::state::filled) {
                m_index.value() += 1;
            }
            if (m_index.value() == m_map->m_slots.size()) {
                m_index = std::nullopt;
            }
        }

        bool validative_check() const {
            if (!m_map) {
                return false;
            }
            if (m_map->m_generation != m_generation) [[unlikely]] {
                return false;
            }
            return true;
        }

        bool end_check() const { return m_index.has_value(); }

        hash_map *m_map;
        usize m_generation;
        usize m_counter{0};
        std::optional<usize> m_index;

        [[no_unique_address]] mutable raw_storage<mut_value_type> m_cache;
        mutable bool m_cache_valid{false};
    };

    class iterator_const {
        friend class hash_map;

        using stored_value_type = fuck_void_or_else<V, detail::pair_ref_const<K, fuck_void<V>>, K>;

    public:
        using difference_type = isize;
        using value_type = stored_value_type;
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;

        iterator_const() = default;

        ~iterator_const() {
            if (m_cache_valid) {
                m_cache.destroy();
            }
        }

        const value_type &operator*() const pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            if (m_cache_valid) {
                m_cache.destroy();
            }
            auto &sl = m_map->m_slots.at(m_index.value());
            if constexpr (std::is_void_v<V>) {
                m_cache.emplace(sl.key);
            } else {
                m_cache.emplace(sl.key, *sl.value.get());
            }
            m_cache_valid = true;
            return *m_cache.get();
        }

        iterator_const &operator++() pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            m_index.value() += 1;
            m_counter += 1;
            m_cache_valid = false;
            next_filled();
            return *this;
        }

        iterator_const operator++(int) pre(validative_check() && end_check()) {
            if (!(validative_check() && end_check())) {
                panic("assertion failed: validative_check() && end_check()");
            }  // TODO: workaround gcc contract bug for template function
            iterator_const res = *this;
            (*this)++;
            return res;
        }

        iterator_const(const iterator_const &other)
                : m_map{other.m_map}
                , m_generation{other.m_generation}
                , m_counter{other.m_counter}
                , m_index{other.m_index} {}

        iterator_const &operator=(const iterator_const &other) {
            if (this == &other) {
                return *this;
            }
            if (m_cache_valid) {
                m_cache.destroy();
            }
            m_map = other.m_map;
            m_generation = other.m_generation;
            m_counter = other.m_counter;
            m_index = other.m_index;
            m_cache_valid = false;
            return *this;
        }

        iterator_const(iterator_const &&other)
                : m_map{other.m_map}
                , m_generation{other.m_generation}
                , m_counter{other.m_counter}
                , m_index{other.m_index} {
            other.m_index = std::nullopt;
            other.m_counter = 0;
            if (other.m_map) {
                other.m_generation = other.m_map->m_generation + 1;
                other.m_map = nullptr;
            } else {
                other.m_generation = other.m_generation + 1;
            }
        }

        iterator_const &operator=(iterator_const &&other) {
            if (this == &other) {
                return *this;
            }
            if (m_cache_valid) {
                m_cache.destroy();
            }
            m_map = other.m_map;
            m_generation = other.m_generation;
            m_counter = other.m_counter;
            m_index = other.m_index;
            m_cache_valid = false;
            other.m_index = std::nullopt;
            other.m_counter = 0;
            if (other.m_map) {
                other.m_generation = other.m_map->m_generation + 1;
                other.m_map = nullptr;
            } else {
                other.m_generation = other.m_generation + 1;
            }
            return *this;
        }

        bool operator==(const iterator_const &rhs) const
            pre(validative_check() && rhs.validative_check() && m_map == rhs.m_map) {
            if (!(validative_check() && rhs.validative_check() && m_map == rhs.m_map)) {
                panic("assertion failed: validative_check() && rhs.validative_check() && m_map == rhs.m_map");
            }  // TODO: workaround gcc contract bug for template function
            return m_index == rhs.m_index;
        }

        friend difference_type operator-(const iterator_const &lhs, const iterator_const &rhs)
            pre(lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map) {
            if (!(lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map)) {
                panic(
                    "assertion failed: lhs.validative_check() && rhs.validative_check() && lhs.m_map == rhs.m_map");
            }  // TODO: workaround gcc contract bug for template function
            return static_cast<difference_type>(lhs.m_counter) - static_cast<difference_type>(rhs.m_counter);
        }

    private:
        iterator_const(const hash_map &map, usize generation, bool end = false)
                : m_map{&map}
                , m_generation{generation}
                , m_counter{end ? map.m_load : 0}
                , m_index{end ? std::optional<usize>{} : std::optional<usize>{0}} {
            if (!end) {
                next_filled();
            }
        }

        void next_filled() {
            if (!m_index.has_value()) {
                return;
            }
            if (m_index.value() == m_map->m_slots.size()) {
                m_index = std::nullopt;
                return;
            }
            while (m_index.value() < m_map->m_slots.size()
                   && m_map->m_slots.at(m_index.value()).st != slot::state::filled) {
                m_index.value() += 1;
            }
            if (m_index.value() == m_map->m_slots.size()) {
                m_index = std::nullopt;
            }
        }

        bool validative_check() const {
            if (!m_map) {
                return false;
            }
            if (m_map->m_generation != m_generation) [[unlikely]] {
                return false;
            }
            return true;
        }

        bool end_check() const { return m_index.has_value(); }

        const hash_map *m_map;
        usize m_generation;
        usize m_counter{0};
        std::optional<usize> m_index;

        [[no_unique_address]] mutable raw_storage<value_type> m_cache;
        mutable bool m_cache_valid{false};
    };

    hash_map() = default;

    iterator begin() { return {*this, m_generation}; }
    iterator_const begin() const { return {*this, m_generation}; }

    iterator end() { return {*this, m_generation, true}; }
    iterator_const end() const { return {*this, m_generation, true}; }

    class view_type : public std::ranges::view_interface<view_type> {
    public:
        view_type() = default;
        explicit view_type(hash_map *m)
                : m_map{m} {}

        iterator begin() { return m_map->begin(); }
        iterator end() { return m_map->end(); }

    private:
        hash_map *m_map{nullptr};
    };

    class view_type_const : public std::ranges::view_interface<view_type_const> {
    public:
        view_type_const() = default;
        explicit view_type_const(const hash_map *m)
                : m_map{m} {}

        iterator_const begin() const { return m_map->begin(); }
        iterator_const end() const { return m_map->end(); }

    private:
        const hash_map *m_map{nullptr};
    };

    view_type view() { return view_type{this}; }
    view_type_const view() const { return view_type_const{this}; }

    usize size() const { return m_load; }

    bool is_empty() const { return m_load == 0; }

    usize capacity() const { return m_slots.size(); }

    void rehash() {
        usize size = m_slots.size();

        bool shoud_extend = m_load * load_factor_denominator > load_factor_numerator * size;
        bool shoud_shrink = m_load * load_factor_denominator * 4 < load_factor_numerator * size;
        if (!shoud_extend && !shoud_shrink) {
            return;
        }

        auto old_slots = std::vector<slot>(shoud_extend ? size * 2 : size / 2);
        m_slots.swap(old_slots);
        m_load = 0;
        for (slot &sl : old_slots) {
            if (sl.st != slot::state::filled) {
                continue;
            }

            if constexpr (std::is_void_v<V>) {
                try_insert(sl.key, std::monostate{});
            } else {
                try_insert(sl.key, try_move(*sl.value.get()));
            }
        }

        m_generation += 1;
    }

    template<typename... Args>
    bool emplace(const K &key, Args &&...args) {
        while (true) {
            switch (try_insert(key, std::forward<Args>(args)...)) {
            case try_insert_result::succeeded:
                m_generation += 1;
                return true;
            case try_insert_result::rehash_needed: {
                rehash();
            } break;
            case try_insert_result::duplicated:
                return false;
            }
        }
    }

    bool insert(const K &key, try_move_t<fuck_void<V>> v) {
        return emplace(key, std::forward<decltype(v)>(v));
    }

    bool insert(const K &key)
        requires std::is_void_v<V>
    {
        return insert(key, std::monostate{});
    }

    std::optional<fuck_void<V>> remove(const K &key) {
        usize size = m_slots.size();
        usize index = m_hasher(key);
        usize step = probe_step(index, size);
        index %= size;

        for (usize i = 0; i < size; ++i) {
            slot &s = m_slots[probe_index(index, step, size, i)];

            switch (s.st) {
            case slot::state::empty: {
                return std::nullopt;
            }
            case slot::state::filled: {
                if (s.key != key) {
                    continue;
                }

                if constexpr (concepts::non_void<V>) {
                    std::optional<V> res = std::move(*s.value.get());
                    s.st = slot::state::tombstone;
                    s.value.destroy();
                    m_load -= 1;
                    m_generation += 1;
                    return res;
                } else {
                    s.st = slot::state::tombstone;
                    m_load -= 1;
                    m_generation += 1;
                    return std::monostate{};
                }
            } break;
            }
        }

        return std::nullopt;
    }

    V *get(const K &key) {
        usize size = m_slots.size();
        usize index = m_hasher(key);
        usize step = probe_step(index, size);
        index %= size;

        for (usize i = 0; i < size; ++i) {
            slot &s = m_slots[probe_index(index, step, size, i)];

            switch (s.st) {
            case slot::state::empty: {
                return nullptr;
            }
            case slot::state::filled: {
                if (s.key != key) {
                    continue;
                }

                return s.value.get();
            } break;
            }
        }

        return nullptr;
    }

    const V *get(const K &key) const {
        usize size = m_slots.size();
        usize index = m_hasher(key);
        usize step = probe_step(index, size);
        index %= size;

        for (usize i = 0; i < size; ++i) {
            const slot &s = m_slots[probe_index(index, step, size, i)];

            switch (s.st) {
            case slot::state::empty: {
                return nullptr;
            }
            case slot::state::filled: {
                if (s.key != key) {
                    continue;
                }

                return s.value.get();
            } break;
            }
        }

        return nullptr;
    }

private:
    static usize probe_step(usize index, usize size) {
        usize step = util::mix64(index) % size;
        return step == 0 ? 1 : step;
    }

    static usize probe_index(usize index, usize step, usize size, usize probe) {
        return (index + probe * step) % size;
    }

    template<typename... Args>
    try_insert_result try_insert(const K &key, Args &&...args) {
        usize size = m_slots.size();

        if (m_load * load_factor_denominator > load_factor_numerator * size) [[unlikely]] {
            return try_insert_result::rehash_needed;
        }

        usize index = m_hasher(key);
        usize step = probe_step(index, size);
        index %= size;

        for (usize i = 0; i < size; ++i) {
            slot &s = m_slots[probe_index(index, step, size, i)];

            switch (s.st) {
            case slot::state::tombstone:
            case slot::state::empty: {
                if constexpr (concepts::non_void<V>) {
                    s.value.emplace(std::forward<Args>(args)...);
                }
                s.key = key;

                s.st = slot::state::filled;
                m_load += 1;
                return try_insert_result::succeeded;
            }
            case slot::state::filled: {
                if (s.key == key) {
                    return try_insert_result::duplicated;
                }
            } break;
            }
        }

        std::unreachable();
    }

    [[no_unique_address]] std::hash<K> m_hasher{};
    std::vector<slot> m_slots{64};
    usize m_load{0};

    usize m_generation{0};
};

template<hash_key K>
using hash_set = hash_map<K, void>;

};  // namespace jungle

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_size<jungle::detail::pair_ref<K, V>> : std::integral_constant<std::size_t, 2> {};

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_size<jungle::detail::pair_ref_const<K, V>> : std::integral_constant<std::size_t, 2> {};

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_element<0, jungle::detail::pair_ref<K, V>> {
    using type = const K &;
};

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_element<1, jungle::detail::pair_ref<K, V>> {
    using type = V &;
};

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_element<0, jungle::detail::pair_ref_const<K, V>> {
    using type = const K &;
};

template<jungle::hash_key K, jungle::concepts::non_void V>
struct std::tuple_element<1, jungle::detail::pair_ref_const<K, V>> {
    using type = const V &;
};
