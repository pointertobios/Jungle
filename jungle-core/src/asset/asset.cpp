#include "jungle/core/asset/asset.h"

namespace jungle::core::asset {

ustr AssetID::debug() const {
    if (!*this) {
        return ustr{"AssetID(NONE)"};
    }
    ustr result{"AssetID("};
    auto range = std::span{reinterpret_cast<const u8 *>(m_id.data()), m_id.size() * sizeof(u64)};
    result.append(util::base64_encoder_view{range});
    result.append(")");
    return result;
}

};  // namespace jungle::core::asset
