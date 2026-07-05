// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <type_traits>

#include "jungle/constants.h"
#include "jungle/preusing.h"

namespace jungle::sync {

template<typename = void, bool = false>
class rwspinlock {
    struct state {
        usize m_reader : (sizeof(usize) - 2);
        bool m_write_willing : 1;
        bool m_writing : 1;
    };

public:
    class write_guard final {
        friend class rwspinlock;

    public:
        write_guard() = default;

        ~write_guard() {
            if (!m_rwspinlock) {
                return;
            }

            auto &s = m_rwspinlock->m_state;
            while (true) {
                state e{0, true, true};
                if (s.compare_exchange_weak(e, state{0, false, false}, morder::acq_rel, morder::relaxed)) {
                    break;
                }
            }
        }

        write_guard(const write_guard &) = delete;
        write_guard &operator=(const write_guard &) = delete;

        write_guard(write_guard &&rhs)
                : m_rwspinlock{std::move(rhs.m_rwspinlock)} {
            rhs.m_rwspinlock = nullptr;
        }

        write_guard &operator=(write_guard &&rhs) {
            if (this != &rhs) {
                this->~write_guard();
                new (this) write_guard{std::move(rhs)};
            }
            return *this;
        }

        operator bool() const { return m_rwspinlock != nullptr; }

    private:
        write_guard(rwspinlock *p)
                : m_rwspinlock{p} {}

        rwspinlock *m_rwspinlock{nullptr};
    };

    class read_guard final {
        friend class rwspinlock;

    public:
        read_guard() = default;

        ~read_guard() {
            if (!m_rwspinlock) {
                return;
            }

            auto &s = m_rwspinlock->m_state;
            while (true) {
                state e = s.load(morder::acquire);
                state i = e;
                i.m_reader -= 1;
                if (s.compare_exchange_weak(
                        e, state{e.m_reader - 1, e.m_write_willing, e.m_writing}, morder::acq_rel,
                        morder::relaxed)) {
                    break;
                }
            }
        }

        read_guard(const read_guard &) = delete;
        read_guard &operator=(const read_guard &) = delete;

        read_guard(read_guard &&rhs)
                : m_rwspinlock{std::move(rhs.m_rwspinlock)} {
            rhs.m_rwspinlock = nullptr;
        }

        read_guard &operator=(read_guard &&rhs) {
            if (this != &rhs) {
                this->~read_guard();
                new (this) read_guard{std::move(rhs)};
            }
            return *this;
        }

        operator bool() const { return m_rwspinlock != nullptr; }

        write_guard upgrade() {
            if (!m_rwspinlock) {
                return write_guard{};
            }

            auto &s = m_rwspinlock->m_state;
            while (true) {
                state e = s.load(morder::acquire);
                if (e.m_write_willing || e.m_writing) {
                    return write_guard{};
                }
                e.m_reader = 1;
                if (s.compare_exchange_weak(e, state{1, true, false}, morder::acq_rel, morder::relaxed)) {
                    break;
                }
            }
            s.store(state{0, true, true}, morder::acq_rel);
            m_rwspinlock = nullptr;
            return write_guard{m_rwspinlock};
        }

    private:
        read_guard(rwspinlock *p)
                : m_rwspinlock{p} {}

        rwspinlock *m_rwspinlock{nullptr};
    };

    rwspinlock() = default;

    rwspinlock(const rwspinlock &) = delete;
    rwspinlock &operator=(const rwspinlock &) = delete;

    rwspinlock(rwspinlock &&) = delete;
    rwspinlock &operator=(rwspinlock &&) = delete;

    read_guard try_read() {
        state e = m_state.load(morder::acquire);
        e.m_write_willing = false;
        e.m_writing = false;
        if (m_state.compare_exchange_strong(
                e, state{e.m_reader + 1, false, false}, morder::acq_rel, morder::relaxed)) {
            return read_guard{this};
        } else {
            return read_guard{};
        }
    }

    write_guard try_write() {
        state e{0, false, false};
        if (m_state.compare_exchange_strong(e, state{0, true, true}, morder::acq_rel, morder::relaxed)) {
            return write_guard{this};
        } else {
            return write_guard{};
        }
    }

    read_guard read() {
        while (true) {
            state e = m_state.load(morder::acquire);
            e.m_write_willing = false;
            e.m_writing = false;
            if (m_state.compare_exchange_weak(
                    e, state{e.m_reader + 1, false, false}, morder::acq_rel, morder::relaxed)) {
                return read_guard{this};
            }
        }
    }

    write_guard write() {
        while (true) {
            state e = m_state.load(morder::acquire);
            e.m_write_willing = false;
            e.m_writing = false;
            if (m_state.compare_exchange_weak(
                    e, state{e.m_reader, true, false}, morder::acq_rel, morder::relaxed)) {
                break;
            }
        }
        while (true) {
            state e{0, true, false};
            if (m_state.compare_exchange_weak(e, state{0, true, true}, morder::acq_rel, morder::relaxed)) {
                break;
            }
        }
        return write_guard{this};
    }

private:
    std::atomic<state> m_state{state{0, false, false}};
};

template<concepts::non_void T, bool ReadMutable>
class rwspinlock<T, ReadMutable> {
public:
    class read_guard final {
        friend class rwspinlock;

    public:
        read_guard() = default;

        ~read_guard() = default;

        read_guard(const read_guard &) = delete;
        read_guard &operator=(const read_guard &) = delete;

        read_guard(read_guard &&rhs) noexcept
                : m_rwspinlock{std::move(rhs.m_rwspinlock)}
                , m_inner_guard{std::move(rhs.m_inner_guard)} {
            rhs.m_rwspinlock = nullptr;
        }

        read_guard &operator=(read_guard &&rhs) noexcept {
            if (this != &rhs) {
                this->~read_guard();
                new (this) read_guard{std::move(rhs)};
            }
            return *this;
        }

        operator bool() const noexcept { return m_rwspinlock != nullptr; }

        T &operator*()
            requires(ReadMutable)
        pre(*this) {
            return m_rwspinlock->m_value;
        }

        const T &operator*() const pre(*this) { return m_rwspinlock->m_value; }

        T *operator->()
            requires(ReadMutable)
        pre(*this) {
            return &m_rwspinlock->m_value;
        }

        const T *operator->() const pre(*this) { return &m_rwspinlock->m_value; }

    private:
        read_guard(rwspinlock *p, rwspinlock<>::read_guard &&inner_guard) noexcept
                : m_rwspinlock{p}
                , m_inner_guard{std::move(inner_guard)} {}

        rwspinlock *m_rwspinlock{nullptr};
        rwspinlock<>::read_guard m_inner_guard;
    };

    class write_guard final {
        friend class rwspinlock;

    public:
        write_guard() = default;

        ~write_guard() = default;

        write_guard(const write_guard &) = delete;
        write_guard &operator=(const write_guard &) = delete;

        write_guard(write_guard &&rhs) noexcept
                : m_rwspinlock{std::move(rhs.m_rwspinlock)} {
            rhs.m_rwspinlock = nullptr;
        }
        write_guard &operator=(write_guard &&rhs) noexcept {
            if (this != &rhs) {
                this->~write_guard();
                new (this) write_guard{std::move(rhs)};
            }
            return *this;
        }

        operator bool() const noexcept { return m_rwspinlock != nullptr; }

        T &operator*() pre(*this) { return m_rwspinlock->m_value; }
        const T &operator*() const pre(*this) { return m_rwspinlock->m_value; }

        T *operator->() pre(*this) { return &m_rwspinlock->m_value; }
        const T *operator->() const pre(*this) { return &m_rwspinlock->m_value; }

    private:
        write_guard(rwspinlock *p, rwspinlock<>::write_guard &&inner_guard) noexcept
                : m_rwspinlock{p}
                , m_inner_guard{std::move(inner_guard)} {}

        rwspinlock *m_rwspinlock{nullptr};
        rwspinlock<>::write_guard m_inner_guard;
    };

    rwspinlock()
        requires(std::is_default_constructible_v<T>)
            : m_value{} {}

    template<typename... Args>
    rwspinlock(Args &&...args)
            : m_value{args...} {}

    rwspinlock(const T &value)
        requires(std::is_copy_constructible_v<T>)
            : m_value{value} {}

    rwspinlock(T &&value)
        requires(std::is_move_constructible_v<T>)
            : m_value{value} {}

    rwspinlock(const rwspinlock &) = delete;
    rwspinlock &operator=(const rwspinlock &) = delete;

    rwspinlock(rwspinlock &&) = delete;
    rwspinlock &operator=(rwspinlock &&) = delete;

    read_guard try_read() {
        if (auto g = m_lock.try_read()) {
            return read_guard{this, std::move(g)};
        } else {
            return read_guard{};
        }
    }

    write_guard try_write() {
        if (auto g = m_lock.try_write()) {
            return write_guard{this, std::move(g)};
        } else {
            return write_guard{};
        }
    }

    read_guard read() { return read_guard{this, m_lock.read()}; }

    write_guard write() { return write_guard{this, m_lock.write()}; }

private:
    T m_value;
    rwspinlock<> m_lock{};
};

};  // namespace jungle::sync
