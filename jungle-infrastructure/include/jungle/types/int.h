// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <tuple>
#include <utility>

namespace jungle {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using u128 = __uint128_t;
using usize = std::size_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using i128 = __int128;
using isize = std::ptrdiff_t;

consteval u8 operator"" _u8(unsigned long long x) { return x; }
consteval u16 operator"" _u16(unsigned long long x) { return x; }
consteval u32 operator"" _u32(unsigned long long x) { return x; }
consteval u64 operator"" _u64(unsigned long long x) { return x; }
consteval usize operator"" _usize(unsigned long long x) { return x; }

consteval u128 operator"" _u128(const char *, usize) { std::unreachable(); }

consteval i8 operator"" _i8(unsigned long long x) { return x; }
consteval i16 operator"" _i16(unsigned long long x) { return x; }
consteval i32 operator"" _i32(unsigned long long x) { return x; }
consteval i64 operator"" _i64(unsigned long long x) { return x; }
consteval isize operator"" _isize(unsigned long long x) { return x; }

consteval i128 operator"" _i128(const char *, usize) { std::unreachable(); }

};  // namespace jungle
