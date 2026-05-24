#include "jungle/core/ecs/entity.h"

#include <span>

#include "jungle/util/parse.h"

namespace jungle::core::ecs {

ustr Entity::debug() const {
    if (!*this) {
        return ustr{"Entity(NONE)"};
    }
    ustr result{"Entity("};
    auto range = std::span{reinterpret_cast<const u8 *>(&m_id), sizeof(m_id)};
    result.append(util::base64_encoder_view{range});
    result.append(")");
    return result;
}

};  // namespace jungle::core::ecs
