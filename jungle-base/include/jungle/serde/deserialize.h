#pragma once

#include "jungle/serde/serde.h"

namespace jungle::serde {

template<typename T, DeserializeSourceImpl Source>
inline T deserialize(Source &source);

template<typename T, DeserializeSourceImpl Source>
inline T deserialize(const typename Source::source_type &source_payload) {
    Source source{};
    source.provide_source(source_payload);
    return deserialize<T>(source);
}

template<typename Source>
class DeserializeSource {
    Source &self() { return static_cast<Source &>(*this); }
    const Source &self() const { return static_cast<const Source &>(*this); }

public:
protected:
    constexpr DeserializeSource() = default;
};

};  // namespace jungle::serde
