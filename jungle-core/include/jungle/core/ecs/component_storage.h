// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <concepts>
#include <deque>
#include <memory>
#include <ranges>
#include <type_traits>

#include "jungle/container/hash_map.h"
#include "jungle/core/ecs/component.h"
#include "jungle/preusing.h"

namespace jungle::core::ecs {

template<typename C, typename StorageType>
class ComponentStorage {
    StorageType &self() { return *static_cast<StorageType *>(this); }
    const StorageType &self() const { return *static_cast<const StorageType *>(this); }

public:
    template<typename... Args>
    C &create(ComponentID id, Args &&...args) {
        return self().create(id, std::forward<Args>(args)...);
    }

    void destroy(ComponentID id) { self().destroy(id); }

    C &get_component(ComponentID id) { return self().get_component(id); }

    const C &get_component(ComponentID id) const { return self().get_component(id); }

    auto get_components() { return self().get_components(); }

    auto get_components() const { return self().get_components(); }

    auto get_components(Entity entity) {
        return get_components() | std::views::filter([entity](C &c) { return c.owner_entity() == entity; });
    }

    auto get_components(Entity entity) const {
        return get_components()
               | std::views::filter([entity](const C &c) { return c.owner_entity() == entity; });
    }
};

template<typename C>
class DenseComponentStorage : public ComponentStorage<C, DenseComponentStorage<C>> {
    static constexpr usize segment_size = 64;
    static constexpr usize segment_size_mask = 0x3f;

    static constexpr usize default_segments = 16;

    union Slot {
        raw_storage<C> component;
        struct {
            Slot *slot;
            usize index;
        } next_free;
    };

    struct Segment {
        std::array<Slot, segment_size> components;
    };

    struct SegmentBitMap {
        std::array<bool, segment_size> map;
    };

    template<bool IsConst>
    class dense_component_view : public std::ranges::view_interface<dense_component_view<IsConst>> {
        friend class DenseComponentStorage;

        using storage_t = std::conditional_t<IsConst, const DenseComponentStorage, DenseComponentStorage>;
        using segments_t = std::conditional_t<IsConst, const std::deque<Segment>, std::deque<Segment>>;
        using bitmaps_t =
            std::conditional_t<IsConst, const std::deque<SegmentBitMap>, std::deque<SegmentBitMap>>;

        segments_t *m_segments = nullptr;
        bitmaps_t *m_bitmaps = nullptr;

        constexpr dense_component_view(segments_t *s, bitmaps_t *b)
                : m_segments{s}
                , m_bitmaps{b} {}

    public:
        using comp_ref = std::conditional_t<IsConst, const C &, C &>;

        constexpr dense_component_view() = default;

        class iterator {
            friend class dense_component_view;

            segments_t *m_segments = nullptr;
            bitmaps_t *m_bitmaps = nullptr;
            usize m_seg_idx = 0;
            usize m_slot_idx = 0;

            constexpr iterator(segments_t *s, bitmaps_t *b, usize seg_idx, usize slot_idx)
                    : m_segments{s}
                    , m_bitmaps{b}
                    , m_seg_idx{seg_idx}
                    , m_slot_idx{slot_idx} {
                skip_to_valid();
            }

            constexpr void skip_to_valid() {
                if (!m_segments || !m_bitmaps) {
                    return;
                }
                while (m_seg_idx < m_segments->size()) {
                    while (m_slot_idx < segment_size) {
                        if ((*m_bitmaps)[m_seg_idx].map[m_slot_idx]) {
                            return;
                        }
                        ++m_slot_idx;
                    }
                    m_slot_idx = 0;
                    ++m_seg_idx;
                }
            }

        public:
            using value_type = C;
            using difference_type = std::ptrdiff_t;
            using iterator_concept = std::forward_iterator_tag;

            constexpr iterator() = default;

            constexpr comp_ref operator*() const {
                return *(*m_segments)[m_seg_idx].components[m_slot_idx].component.get();
            }

            constexpr iterator &operator++() {
                ++m_slot_idx;
                skip_to_valid();
                return *this;
            }

            constexpr iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            constexpr bool operator==(const iterator &other) const = default;
        };

        constexpr iterator begin() { return iterator{m_segments, m_bitmaps, 0, 0}; }
        constexpr iterator end() {
            return iterator{m_segments, m_bitmaps, m_segments ? m_segments->size() : 0, 0};
        }
    };

public:
    DenseComponentStorage()
            : m_storage(default_segments)
            , m_bitmaps(default_segments) {
        template for (constexpr auto i : std::views::iota(0u, segment_size)) {
            template for (constexpr auto j : std::views::iota(0u, default_segments - 1)) {
                m_storage[j].components[i].next_free = {&m_storage[j + 1].components[i], j + 1};
            }
            m_storage[default_segments - 1].components[i].next_free = {nullptr, 0};
            new (&m_free_list_heads[i]) Slot{.next_free = {&m_storage[0].components[i], 0}};
        }
    }

    template<typename... Args>
    C &create(ComponentID id, Args &&...args) {
        usize seg_index = segment_index(id);
        auto [slot, index] = m_free_list_heads[seg_index].next_free;
        if (!slot) {
            index = m_storage.size();
            m_storage.emplace_back();
            m_bitmaps.emplace_back();
            slot = &m_storage.back().components[seg_index];
            template for (constexpr auto i : std::views::iota(0u, segment_size)) {
                if (i != seg_index) {
                    auto &slot = m_storage.back().components[i];
                    slot.next_free = m_free_list_heads[i].next_free;
                    new (&m_free_list_heads[i]) Slot{.next_free = {&slot, index}};
                }
            }
        } else {
            new (&m_free_list_heads[seg_index])
                Slot{.next_free = {slot->next_free.slot, slot->next_free.index}};
        }
        m_segment_of_component.insert(id, index);
        m_bitmaps[index].map[seg_index] = true;
        return slot->component.emplace(std::forward<Args>(args)...);
    }

    void destroy(ComponentID id) {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        contract_assert(index != nullptr);
        m_storage[*index].components[seg_index].component.destroy();
        m_bitmaps[*index].map[seg_index] = false;
        new (&m_free_list_heads[seg_index])
            Slot{.next_free = {&m_storage[*index].components[seg_index], *index}};
        m_segment_of_component.remove(id);
    }

    C &get_component(ComponentID id) {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        contract_assert(index != nullptr);
        return *m_storage[*index].components[seg_index].component.get();
    }

    const C &get_component(ComponentID id) const {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        contract_assert(index != nullptr);
        return *m_storage[*index].components[seg_index].component.get();
    }

    auto get_components() { return dense_component_view<false>{&m_storage, &m_bitmaps}; }

    auto get_components() const { return dense_component_view<true>{&m_storage, &m_bitmaps}; }

    using ComponentStorage<C, DenseComponentStorage<C>>::get_components;

private:
    usize segment_index(ComponentID id) const { return id.underlying() & segment_size_mask; }

    std::deque<Segment> m_storage;  // ！！！ 禁止 insert / erase 以及对 front 的操作
    std::deque<SegmentBitMap> m_bitmaps;
    std::array<Slot, segment_size> m_free_list_heads;
    hash_map<ComponentID, usize> m_segment_of_component;
};

template<typename C>
class SparseComponentStorage : public ComponentStorage<C, SparseComponentStorage<C>> {
public:
    SparseComponentStorage() = default;

    template<typename... Args>
    C &create(ComponentID id, Args &&...args) {
        auto ptr = std::make_unique<C>(std::forward<Args>(args)...);
        C &ref = *ptr;
        m_components.emplace(id, std::move(ptr));
        return ref;
    }

    void destroy(ComponentID id) { m_components.remove(id); }

    C &get_component(ComponentID id) {
        auto ptr = m_components.get(id);
        contract_assert(ptr != nullptr);
        return **ptr;
    }

    const C &get_component(ComponentID id) const {
        auto ptr = m_components.get(id);
        contract_assert(ptr != nullptr);
        return **ptr;
    }

    auto get_components() {
        return m_components.view() | std::views::transform([](const auto &kv) -> C & { return *kv.value(); });
    }

    auto get_components() const {
        return m_components.view()
               | std::views::transform([](const auto &kv) -> const C & { return *kv.value(); });
    }

    using ComponentStorage<C, SparseComponentStorage<C>>::get_components;

private:
    hash_map<ComponentID, std::unique_ptr<C>> m_components;
};

};  // namespace jungle::core::ecs
