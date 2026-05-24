#pragma once

#include <array>
#include <concepts>
#include <deque>
#include <memory>
#include <ranges>
#include <type_traits>

#include "jungle/container/hash_map.h"
#include "jungle/core/ecs/component.h"
#include "jungle/types/raw_storage.h"

namespace jungle::core::ecs {

template<ComponentImpl C, typename StorageType>
    requires(
        std::derived_from<StorageType, ComponentStorage<C, StorageType>>
        && !std::same_as<StorageType, ComponentStorage<C, StorageType>>)
class ComponentStorage<C, StorageType> {
    template<typename... Args>
    static constexpr bool nothrow = std::is_nothrow_constructible_v<C, Args...>;

public:
    template<typename... Args>
    C &create(ComponentID id, Args &&...args) noexcept(nothrow<decltype(args)...>) {
        auto self = static_cast<StorageType *>(this);
        return self->create(id, std::forward<Args>(args)...);
    }

    void destroy(ComponentID id) {
        auto self = static_cast<StorageType *>(this);
        self->destroy(id);
    }

    C &get_component(ComponentID id) {
        auto self = static_cast<StorageType *>(this);
        return self->get_component(id);
    }

    const C &get_component(ComponentID id) const {
        auto self = static_cast<const StorageType *>(this);
        return self->get_component(id);
    }

    auto get_components() {
        auto self = static_cast<StorageType *>(this);
        return self->get_components();
    }

    auto get_components() const {
        auto self = static_cast<const StorageType *>(this);
        return self->get_components();
    }

    auto get_components(Entity entity) {
        return get_components() | std::views::filter([entity](C &c) { return c.owner_entity() == entity; });
    }

    auto get_components(Entity entity) const {
        return get_components()
               | std::views::filter([entity](const C &c) { return c.owner_entity() == entity; });
    }
};

template<ComponentImpl C>
class DenseComponentStorage : public ComponentStorage<C, DenseComponentStorage<C>> {
    template<typename... Args>
    static constexpr bool nothrow = std::is_nothrow_constructible_v<C, Args...>;

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

public:
    DenseComponentStorage()
            : m_storage(default_segments)
            , m_bitmaps(default_segments) {
        template for (constexpr auto i : std::views::iota(0u, segment_size)) {
            template for (constexpr auto j : std::views::iota(0u, default_segments - 1)) {
                m_storage[j].components[i].next_free = {&m_storage[j + 1].components[i], j};
            }
            m_storage[default_segments - 1].components[i].next_free = {nullptr, 0};
            m_free_list_heads[i] = {.next_free = {&m_storage[0].components[i], 0}};
        }
    }

    template<typename... Args>
    C &create(ComponentID id, Args &&...args) noexcept(nothrow<decltype(args)...>) {
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
                    m_free_list_heads[i] = {&slot, index};
                }
            }
        } else {
            m_free_list_heads[seg_index] = {slot.next_free.slot, slot.next_free.index};
        }
        m_segment_of_component.insert(id, index);
        m_bitmaps[index].map[seg_index] = true;
        return slot->component.emplace(std::forward<Args>(args)...);
    }

    void destroy(ComponentID id) {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        m_storage[index].components[seg_index].component.destroy();
        m_bitmaps[index].map[seg_index] = false;
        m_free_list_heads[seg_index] = {&m_storage[index].components[seg_index], index};
        m_segment_of_component.remove(id);
    }

    C &get_component(ComponentID id) {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        return m_storage[index].components[seg_index].component.get();
    }

    const C &get_component(ComponentID id) const {
        usize seg_index = segment_index(id);
        auto index = m_segment_of_component.get(id);
        return m_storage[index].components[seg_index].component.get();
    }

    auto get_components() {
        return std::views::zip_transform(
                   [](auto &p) {
                       auto &[segment, bitmap] = p;
                       return std::views::zip(segment.components, bitmap.map)
                              | std::views::filter([](const auto &p) {
                                    auto &[_, valid] = p;
                                    return valid;
                                })
                              | std::views::transform([](auto &p) -> C & {
                                    auto &[slot, _] = p;
                                    return *slot.component.get();
                                });
                   },
                   m_storage, m_bitmaps)
               | std::views::join;
    }

    auto get_components() const {
        return std::views::zip_transform(
                   [](auto &p) {
                       auto &[segment, bitmap] = p;
                       return std::views::zip(segment.components, bitmap.map)
                              | std::views::filter([](const auto &p) {
                                    auto &[_, valid] = p;
                                    return valid;
                                })
                              | std::views::transform([](auto &p) -> const C & {
                                    auto &[slot, _] = p;
                                    return *slot.component.get();
                                });
                   },
                   m_storage, m_bitmaps)
               | std::views::join;
    }

private:
    usize segment_index(ComponentID id) { return id.underlying() & segment_size_mask; }

    std::deque<Segment> m_storage;  // ！！！ 禁止 insert / erase 以及对 front 的操作
    std::deque<SegmentBitMap> m_bitmaps;
    std::array<Slot, segment_size> m_free_list_heads;
    hash_map<ComponentID, usize> m_segment_of_component;
};

template<ComponentImpl C>
class SparseComponentStorage : public ComponentStorage<C, SparseComponentStorage<C>> {
public:
    SparseComponentStorage() = default;

    template<typename... Args>
    C &create(ComponentID id, Args &&...args) noexcept(std::is_nothrow_constructible_v<C, Args...>) {
        auto ptr = std::make_unique<C>(std::forward<Args>(args)...);
        C &ref = *ptr;
        m_components.emplace(id, std::move(ptr));
        return ref;
    }

    void destroy(ComponentID id) { m_components.remove(id); }

    C &get_component(ComponentID id) { return *m_components.get(id); }

    const C &get_component(ComponentID id) const { return *m_components.get(id); }

    auto get_components() {
        return m_components | std::views::values
               | std::views::transform([](const auto &ptr) -> C & { return *ptr; });
    }

    auto get_components() const {
        return m_components | std::views::values
               | std::views::transform([](const auto &ptr) -> const C & { return *ptr; });
    }

private:
    hash_map<ComponentID, std::unique_ptr<C>> m_components;
};

};  // namespace jungle::core::ecs
