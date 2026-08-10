// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/shared_memory.h"

#include <atomic>
#include <optional>
#include <string>
#include <string_view>

#ifndef NOMINMAX
#    define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "jungle/constants.h"

namespace jungle::os {

namespace {

struct shm_header {
    usize size;
    std::atomic<usize> holder_count;
};

static_assert(
    std::atomic<usize>::is_always_lock_free, "std::atomic<usize> 必须是无锁的才能用于共享内存进程间通信");

struct win_shm {
    HANDLE mapping;
    void *addr;
    usize total_size;
};

constexpr usize header_size() { return sizeof(shm_header); }

usize calc_total(usize user_size) { return header_size() + user_size; }

shm_header *get_header(void *addr) { return static_cast<shm_header *>(addr); }

void *get_user_data(void *addr) { return static_cast<i8 *>(addr) + header_size(); }

std::wstring to_wstring(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result(len, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), len);
    return result;
}

};  // namespace

std::optional<shared_memory> shared_memory::create(ustr name, usize size) {
    std::wstring wname = to_wstring(name.view());
    usize total = calc_total(size);

    HANDLE h = ::CreateFileMappingW(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, static_cast<DWORD>(total >> 32),
        static_cast<DWORD>(total & 0xFFFFFFFF), wname.c_str());

    if (!h) {
        return std::nullopt;
    }

    bool already_exists = (::GetLastError() == ERROR_ALREADY_EXISTS);
    if (already_exists) {
        ::CloseHandle(h);
        return std::nullopt;
    }

    void *addr = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, total);
    if (!addr) {
        ::CloseHandle(h);
        return std::nullopt;
    }

    new (addr) shm_header{size, 1};

    win_shm shm{h, addr, total};
    return shared_memory{}.with_extra(erased{std::move(shm)});
}

std::optional<shared_memory> shared_memory::attach(ustr name) {
    std::wstring wname = to_wstring(name.view());

    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, wname.c_str());
    if (!h) {
        return std::nullopt;
    }

    void *addr = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!addr) {
        ::CloseHandle(h);
        return std::nullopt;
    }

    auto *hdr = get_header(addr);
    hdr->holder_count.fetch_add(1, morder::relaxed);

    usize total = calc_total(hdr->size);

    win_shm shm{h, addr, total};
    return shared_memory{}.with_extra(erased{std::move(shm)});
}

shared_memory::shared_memory() = default;

shared_memory::~shared_memory() {
    if (!m_extra) {
        return;
    }

    auto &shm = m_extra.get<win_shm>();
    auto *hdr = get_header(shm.addr);

    hdr->holder_count.fetch_sub(1, morder::relaxed);

    ::UnmapViewOfFile(shm.addr);
    ::CloseHandle(shm.mapping);
}

usize shared_memory::size() const {
    auto &shm = m_extra.get<win_shm>();
    return get_header(shm.addr)->size;
}

void *shared_memory::get() const {
    auto &shm = m_extra.get<win_shm>();
    return get_user_data(shm.addr);
}

};  // namespace jungle::os
