#include "jungle/util/parse.h"

namespace jungle::util {

namespace {

constexpr std::array<i8, 64> BASE64_ALPHABET{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                                             'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                                             'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                                             'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                             '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

}  // namespace

uchar base64_encoder_view::iterator::operator*() const { return m_view->at(m_index); }

base64_encoder_view::iterator &base64_encoder_view::iterator::operator++() {
    ++m_index;
    return *this;
}

base64_encoder_view::iterator base64_encoder_view::iterator::operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
}

base64_encoder_view::iterator base64_encoder_view::begin() const { return iterator{this, 0}; }

base64_encoder_view::iterator base64_encoder_view::end() const { return iterator{this, size()}; }

usize base64_encoder_view::size() const {
    const auto n = m_n;
    return static_cast<usize>(((n + 2) / 3) * 4);
}

uchar base64_encoder_view::at(usize index) const {
    const auto block = index / 4;
    const auto offset = index % 4;
    const auto byte_index = block * 3;

    const auto read_byte = [this](usize i) -> u8 {
        if (m_data && i < m_n) {
            return m_data[i];
        }
        return u8{0};
    };

    const auto b0 = read_byte(byte_index);
    const auto b1 = read_byte(byte_index + 1);
    const auto b2 = read_byte(byte_index + 2);

    switch (offset) {
    case 0:
        return uchar{BASE64_ALPHABET[b0 >> 2]};
    case 1:
        return uchar{BASE64_ALPHABET[((b0 & 0x03) << 4) | (b1 >> 4)]};
    case 2:
        if (byte_index + 1 >= m_n) {
            return uchar{'='};
        }
        return uchar{BASE64_ALPHABET[((b1 & 0x0F) << 2) | (b2 >> 6)]};
    default:
        if (byte_index + 2 >= m_n) {
            return uchar{'='};
        }
        return uchar{BASE64_ALPHABET[b2 & 0x3F]};
    }
}

};  // namespace jungle::util
