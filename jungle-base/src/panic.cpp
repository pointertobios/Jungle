#include "jungle/panic.h"

#include <exception>
#include <print>

namespace jungle {

[[noreturn]] void panic(ustr message) {
    std::println("[panic] {}", message.view());
    std::terminate();
}

};  // namespace jungle
