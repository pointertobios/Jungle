// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/os/shared_memory.h"

#include <atomic>
#include <optional>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jungle/build_id.h"
#include "jungle/constants.h"

namespace jungle::os {

namespace {

struct linux_shm {
    std::string name;
    void *addr;
    usize total_size;
};

constexpr usize header_size() { return sizeof(shm_header); }

usize calc_total(usize user_size) { return header_size() + user_size; }

shm_header *get_header(void *addr) { return static_cast<shm_header *>(addr); }

void *get_user_data(void *addr) { return static_cast<i8 *>(addr) + header_size(); }

};  // namespace

std::optional<shared_memory> shared_memory::create(ustr name, usize size) {
    std::string name_str{name.view()};
    usize total = calc_total(size);

    int fd = ::shm_open(name_str.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        return std::nullopt;
    }

    if (::ftruncate(fd, static_cast<off_t>(total)) == -1) {
        ::close(fd);
        ::shm_unlink(name_str.c_str());
        return std::nullopt;
    }

    void *addr = ::mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        ::close(fd);
        ::shm_unlink(name_str.c_str());
        return std::nullopt;
    }

    new (addr) shm_header{size, 1, jungle::build_id()};

    ::close(fd);

    linux_shm shm{
        std::move(name_str),
        addr,
        total,
    };

    return shared_memory{true}.with_extra(erased{std::move(shm)});
}

std::optional<shared_memory> shared_memory::attach(ustr name) {
    std::string name_str{name.view()};

    int fd = ::shm_open(name_str.c_str(), O_RDWR, 0);
    if (fd == -1) {
        return std::nullopt;
    }

    struct stat st;
    if (::fstat(fd, &st) == -1) {
        ::close(fd);
        return std::nullopt;
    }

    auto total = static_cast<usize>(st.st_size);

    void *addr = ::mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        ::close(fd);
        return std::nullopt;
    }

    auto *hdr = get_header(addr);
    if (hdr->build_id != jungle::build_id()) {
        ::munmap(addr, total);
        ::close(fd);
        return std::nullopt;
    }
    hdr->holder_count.fetch_add(1, morder::relaxed);

    ::close(fd);

    linux_shm shm{
        std::move(name_str),
        addr,
        total,
    };

    return shared_memory{false}.with_extra(erased{std::move(shm)});
}

shared_memory::~shared_memory() {
    if (!m_extra) {
        return;
    }

    auto &shm = m_extra.get<linux_shm>();
    auto *hdr = get_header(shm.addr);

    usize cnt = hdr->holder_count.fetch_sub(1, morder::relaxed) - 1;

    if (cnt == 0) {
        if (m_dtor) {
            m_dtor(get());
        }
        ::shm_unlink(shm.name.c_str());
    }

    ::munmap(shm.addr, shm.total_size);
}

usize shared_memory::size() const {
    auto &shm = m_extra.get<linux_shm>();
    return get_header(shm.addr)->size;
}

void *shared_memory::get() const {
    auto &shm = m_extra.get<linux_shm>();
    return get_user_data(shm.addr);
}

};  // namespace jungle::os
