#pragma once

#include "jungle/meta.h"
#include "jungle/types/uchar.h"

namespace jungle {

inline constexpr struct {
} Debug;

template<typename T>
ustr debug_of(const T &value) {
    static_assert(jungle::meta::has_annotation(^^T, Debug), "Type must be annotated with [[=jungle::Debug]] to use debug_of");
    if constexpr (jungle::concepts::DebugType<T>) {
        return value.debug();
    } else {
        return ustr{"<Debug not implemented>"};
    }
}

};  // namespace jungle
