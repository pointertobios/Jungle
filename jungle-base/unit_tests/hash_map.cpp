#include <print>

#include "jungle/container/hash_map.h"
#include "jungle/panic.h"

using namespace jungle;

int main() {
    hash_map<int, int> m;
    for (usize i = 0; i < 1000; ++i) {
        m.insert(i, i * 2);
    }
    for (auto [k, v] : m) {
        jungle_assert(k * 2 == v);
        std::println("{}: {}", k, v);
    }

    hash_set<usize> s;
    for (usize i = 0; i < 1000; ++i) {
        s.insert(i);
    }
    for (auto k : s) {
        jungle_assert(k < 1000);
        std::println("{}", k);
    }
}
