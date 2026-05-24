#include "jungle/core/ecs/component.h"

#include <span>

namespace jungle::core::ecs {

ustr ComponentID::debug() const {
    if (!*this) {
        return ustr{"ComponentID(NONE)"};
    }
    ustr result{"ComponentID("};
    auto range = std::span{reinterpret_cast<const u8 *>(&m_id), sizeof(m_id)};
    result.append(util::base64_encoder_view{range});
    result.append(")");
    return result;
}

};  // namespace jungle::core::ecs
