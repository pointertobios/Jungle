#include "jungle/panic.h"

#include <string>

namespace jungle {

[[noreturn]] void panic(ustr message) { throw std::runtime_error{std::string{message.view()}}; }

};  // namespace jungle
