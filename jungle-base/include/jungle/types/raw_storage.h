#pragma once

#include <new>

#include "jungle/types/int.h"

namespace jungle {

template<typename T>
class raw_storage {
public:
    raw_storage() = default;
    ~raw_storage() = default;

    raw_storage(const raw_storage &) = delete;
    raw_storage &operator=(const raw_storage &) = delete;

    raw_storage(raw_storage &&) = delete;
    raw_storage &operator=(raw_storage &&) = delete;

    template<typename... Args>
    T &emplace(Args &&...args) {
        return *new (std::launder(storage)) T(std::forward<Args>(args)...);
    }

    void destroy() { std::launder(reinterpret_cast<T *>(storage))->~T(); }

    T *get() { return reinterpret_cast<T *>(storage); }
    const T *get() const { return reinterpret_cast<const T *>(storage); }

private:
    alignas(alignof(T)) unsigned char storage[sizeof(T)];
};

template<>
class raw_storage<void> {};

};  // namespace jungle
