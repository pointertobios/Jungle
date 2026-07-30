// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#include "jungle/sync/condition_variable.h"
#include "jungle/tasks/this_task.h"
#include <vector>

namespace jungle::sync {

condition_variable::condition_variable() {
    auto guard = condition_variable::s_parking_lot.write();
    guard->emplace(this, std::vector<awake_token>{});
}

condition_variable::~condition_variable() {
    auto guard = condition_variable::s_parking_lot.write();
    guard->remove(this);
}

bool condition_variable::notify_one() {
    std::vector<awake_token> av;
    {
        auto g = m_suspend_lock.lock();
        if (auto tk = m_fast_awake_token.exchange(awake_token::none(), morder::acq_rel)) {
            av.push_back(tk);
        } else {
            auto guard = condition_variable::s_parking_lot.read();
            if (auto v = guard->get(this)) {
                if (!v->empty()) {
                    auto tk = v->back();
                    v->pop_back();
                    av.push_back(tk);
                    return true;
                }
            }
        }
    }
    if (av.empty()) {
        return false;
    }
    for (auto &tk : av) {
        tk.awake();
    }
    return true;
}

usize condition_variable::notify_all() {
    std::vector<awake_token> av;
    {
        auto g = m_suspend_lock.lock();
        if (auto tk = m_fast_awake_token.exchange(awake_token::none(), morder::acq_rel)) {
            av.push_back(tk);
        }
        auto guard = condition_variable::s_parking_lot.read();
        if (auto v = guard->get(this)) {
            while (!v->empty()) {
                auto tk = v->back();
                v->pop_back();
                av.push_back(tk);
            }
        }
    }
    for (auto &tk : av) {
        tk.awake();
    }
    return av.size();
}

};  // namespace jungle::sync
