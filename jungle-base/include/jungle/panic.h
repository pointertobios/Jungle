#pragma once

#include <format>

#include "jungle/types/uchar.h"

namespace jungle {

[[noreturn]] void panic(ustr message);
[[noreturn]] inline void panic() { panic({}); }

template<typename... Args>
[[noreturn]] inline void panic(std::format_string<fmt::format_arg_t<Args>...> fmt, Args &&...args) {
    panic(ustr{std::vformat(fmt.get(), std::make_format_args(fmt::normalize_format_arg(args)...))});
}

};  // namespace jungle

#ifdef NDEBUG

#    define jungle_assert(expr, ...)                                                          \
        do {                                                                                  \
            if (!(expr)) [[unlikely]] {                                                       \
                auto msg = jungle::ustr(__VA_ARGS__);                                         \
                jungle::panic("'{}' assert failed{}{}", #expr, msg.empty() ? "" : ": ", msg); \
            }                                                                                 \
        } while (0)

#else

#    define jungle_assert(expr, ...) (void)(expr);

#endif
