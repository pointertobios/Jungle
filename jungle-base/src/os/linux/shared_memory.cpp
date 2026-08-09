// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/constants.h"
#include "jungle/os/shared_memory.h"

#include <atomic>
#include <optional>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace jungle::os {

namespace {

struct shm_header {
    usize size;
    std::atomic<usize> holder_count;
};

static_assert(std::atomic<usize>::is_always_lock_free,
              "std::atomic<usize> 必须是无锁的才能用于共享内存进程间通信");

struct linux_shm {
    std::string name;
    void *addr;
    usize total_size;
};

constexpr usize header_size() {
    return sizeof(shm_header);
}

usize calc_total(usize user_size) {
    return header_size() + user_size;
}

shm_header *get_header(void *addr) {
    return static_cast<shm_header *>(addr);
}

void *get_user_data(void *addr) {
    return static_cast<i8 *>(addr) + header_size();
}

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

    new (addr) shm_header{size, 1};

    ::close(fd);

    linux_shm shm{
        std::move(name_str),
        addr,
        total,
    };

    return shared_memory{}.with_extra(erased{std::move(shm)});
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
    hdr->holder_count.fetch_add(1, morder::relaxed);

    ::close(fd);

    linux_shm shm{
        std::move(name_str),
        addr,
        total,
    };

    return shared_memory{}.with_extra(erased{std::move(shm)});
}

shared_memory::shared_memory() = default;

shared_memory::~shared_memory() {
    if (!m_extra) return;

    auto &shm = m_extra.get<linux_shm>();
    auto *hdr = get_header(shm.addr);

    usize prev = hdr->holder_count.fetch_sub(1, morder::relaxed) - 1;

    if (prev == 0) {
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
