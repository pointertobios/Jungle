#include "jungle/core/ecs/entity.h"

#include "jungle/util/parse.h"

namespace jungle::core::ecs {

ustr Entity::debug() const {
    if (!*this) {
        return ustr{"Entity(NONE)"};
    }
    ustr result{"Entity("};
    result.append(util::base64_encoder_view{m_id});
    result.append(")");
    return result;
}

};  // namespace jungle::core::ecs
