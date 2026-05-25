#include "jungle/container/hash_map.h"
#include "jungle/test/test.h"

#include <set>
#include <string>
#include <utility>

using jungle::hash_map;
using jungle::hash_set;

JUNGLE_SYNC_TEST(hash_map_default_state) {
	hash_map<int, int> map;

	JUNGLE_SYNC_ASSERT(map.size() == 0, "default constructed map should be empty");
	JUNGLE_SYNC_ASSERT(map.capacity() == 64, "default constructed map should allocate the default slot count");
	JUNGLE_SYNC_ASSERT(map.get(1) == nullptr, "missing keys should return a null pointer");
	JUNGLE_SYNC_ASSERT(map.begin() == map.end(), "empty maps should not expose any iterable entries");
	return {};
}

JUNGLE_SYNC_TEST(hash_map_insert_get_and_duplicate_insert) {
	hash_map<int, int> map;

	JUNGLE_SYNC_ASSERT(map.insert(1, 10), "first insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(2, 20), "second insert should succeed");
	JUNGLE_SYNC_ASSERT(!map.insert(1, 99), "duplicate insert should fail");
	JUNGLE_SYNC_ASSERT(map.size() == 2, "duplicate insert must not change size");

	auto *first = map.get(1);
	auto *second = map.get(2);
	JUNGLE_SYNC_ASSERT(first != nullptr && *first == 10, "original value should stay retrievable after duplicate insert");
	JUNGLE_SYNC_ASSERT(second != nullptr && *second == 20, "second inserted value should be retrievable");
	return {};
}

JUNGLE_SYNC_TEST(hash_map_emplace_constructs_values_in_place) {
	hash_map<int, std::pair<int, int>> map;

	JUNGLE_SYNC_ASSERT(map.emplace(7, 3, 9), "emplace should construct values directly from forwarded arguments");

	auto *value = map.get(7);
	JUNGLE_SYNC_ASSERT(value != nullptr, "emplaced value should be retrievable");
	JUNGLE_SYNC_ASSERT(value->first == 3 && value->second == 9, "emplace should preserve constructor arguments");
	return {};
}

JUNGLE_SYNC_TEST(hash_map_remove_missing_and_tombstone_reuse) {
	hash_map<int, std::string> map;

	JUNGLE_SYNC_ASSERT(map.insert(1, std::string{"one"}), "initial insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(2, std::string{"two"}), "initial insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(3, std::string{"three"}), "initial insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(4, std::string{"four"}), "initial insert should succeed");

	auto removed = map.remove(2);
	JUNGLE_SYNC_ASSERT(removed.has_value(), "remove should return the erased value for existing keys");
	JUNGLE_SYNC_ASSERT(*removed == "two", "remove should return the exact erased value");
	JUNGLE_SYNC_ASSERT(map.get(2) == nullptr, "removed keys should no longer be retrievable");
	JUNGLE_SYNC_ASSERT(map.size() == 3, "remove should decrease the map size");
	JUNGLE_SYNC_ASSERT(!map.remove(99).has_value(), "removing a missing key should return nullopt");

	JUNGLE_SYNC_ASSERT(map.insert(5, std::string{"five"}), "tombstone slots should remain reusable for later inserts");
	auto *value = map.get(5);
	JUNGLE_SYNC_ASSERT(value != nullptr && *value == "five", "inserts after remove should remain retrievable");
	JUNGLE_SYNC_ASSERT(map.size() == 4, "reinserting after removal should restore the size");
	return {};
}

JUNGLE_SYNC_TEST(hash_map_growth_rehash_preserves_entries) {
	hash_map<int, int> map;

	for (int key = 1; key <= 80; ++key) {
		JUNGLE_SYNC_ASSERT(map.insert(key, key * 10), "all inserts before and after growth rehash should succeed");
	}

	JUNGLE_SYNC_ASSERT(map.size() == 80, "all inserted entries should contribute to the load");
	JUNGLE_SYNC_ASSERT(map.capacity() > 64, "crossing the load threshold should grow the backing storage");

	for (int key = 1; key <= 80; ++key) {
		auto *value = map.get(key);
		JUNGLE_SYNC_ASSERT(value != nullptr && *value == key * 10, "growth rehash must preserve every key-value pair");
	}
	return {};
}

JUNGLE_SYNC_TEST(hash_map_manual_rehash_can_shrink_sparse_maps) {
	hash_map<int, int> map;

	for (int key = 1; key <= 80; ++key) {
		JUNGLE_SYNC_ASSERT(map.insert(key, key + 100), "setup inserts should succeed before shrink testing");
	}

	auto expanded_capacity = map.capacity();
	for (int key = 5; key <= 80; ++key) {
		auto removed = map.remove(key);
		JUNGLE_SYNC_ASSERT(removed.has_value(), "setup removals should erase the requested keys");
	}

	map.rehash();

	JUNGLE_SYNC_ASSERT(map.size() == 4, "manual rehash should preserve the remaining element count");
	JUNGLE_SYNC_ASSERT(map.capacity() < expanded_capacity, "manual rehash should shrink sparse storage");
	for (int key = 1; key <= 4; ++key) {
		auto *value = map.get(key);
		JUNGLE_SYNC_ASSERT(value != nullptr && *value == key + 100, "manual shrink rehash must preserve remaining entries");
	}
	return {};
}

JUNGLE_SYNC_TEST(hash_map_iteration_visits_every_entry_and_allows_mutation) {
	hash_map<int, int> map;

	JUNGLE_SYNC_ASSERT(map.insert(1, 10), "setup insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(2, 20), "setup insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(3, 30), "setup insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(4, 40), "setup insert should succeed");

	std::set<int> visited;
	for (auto [key, value] : map) {
		visited.insert(key);
		value += 1;
	}

	std::set<int> expected{1, 2, 3, 4};
	JUNGLE_SYNC_ASSERT(visited == expected, "iteration should visit each inserted key exactly once");
	JUNGLE_SYNC_ASSERT(*map.get(1) == 11, "mutable iteration should expose writable values");
	JUNGLE_SYNC_ASSERT(*map.get(2) == 21, "mutable iteration should expose writable values");
	JUNGLE_SYNC_ASSERT(*map.get(3) == 31, "mutable iteration should expose writable values");
	JUNGLE_SYNC_ASSERT(*map.get(4) == 41, "mutable iteration should expose writable values");
	return {};
}

JUNGLE_SYNC_TEST(hash_map_const_access_and_const_iteration) {
	hash_map<int, int> map;

	JUNGLE_SYNC_ASSERT(map.insert(1, 4), "setup insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(2, 5), "setup insert should succeed");
	JUNGLE_SYNC_ASSERT(map.insert(3, 6), "setup insert should succeed");

	const auto &const_map = map;
	auto *value = const_map.get(2);
	JUNGLE_SYNC_ASSERT(value != nullptr && *value == 5, "const get should expose existing values");

	std::set<int> visited;
	int sum = 0;
	for (auto [key, current] : const_map) {
		visited.insert(key);
		sum += current;
	}

	std::set<int> expected{1, 2, 3};
	JUNGLE_SYNC_ASSERT(visited == expected, "const iteration should visit each inserted key");
	JUNGLE_SYNC_ASSERT(sum == 15, "const iteration should expose the stored values");
	return {};
}

JUNGLE_SYNC_TEST(hash_set_insert_duplicate_and_iteration) {
	hash_set<int> set;

	JUNGLE_SYNC_ASSERT(set.insert(3), "first insert should succeed for hash_set");
	JUNGLE_SYNC_ASSERT(set.insert(7), "second insert should succeed for hash_set");
	JUNGLE_SYNC_ASSERT(set.insert(11), "third insert should succeed for hash_set");
	JUNGLE_SYNC_ASSERT(!set.insert(7), "duplicate keys should be rejected for hash_set as well");
	JUNGLE_SYNC_ASSERT(set.size() == 3, "hash_set should only count unique keys");

	std::set<int> visited;
	for (int key : set) {
		visited.insert(key);
	}

	std::set<int> expected{3, 7, 11};
	JUNGLE_SYNC_ASSERT(visited == expected, "hash_set iteration should expose each unique key exactly once");
	return {};
}
