#include "jungle/core/ecs/component.h"

namespace jungle::core::ecs {

ustr ComponentID::debug() const {
    if (!*this) {
        return ustr{"ComponentID(NONE)"};
    }
    ustr result{"ComponentID("};
    result.append(util::base64_encoder_view{m_id});
    result.append(")");
    return result;
}

};  // namespace jungle::core::ecs
