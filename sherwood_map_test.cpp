#include "sherwood_map.hpp"

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
static_assert(std::is_nothrow_move_constructible<thin_sherwood_map<int, int> >::value, "expect it to be nothrow movable");
static_assert(std::is_nothrow_move_assignable<thin_sherwood_map<int, int> >::value, "expect it to be nothrow movable");
static_assert(std::is_nothrow_move_constructible<fat_sherwood_map<int, int> >::value, "expect it to be nothrow movable");
static_assert(std::is_nothrow_move_assignable<fat_sherwood_map<int, int> >::value, "expect it to be nothrow movable");
#include "sherwood_test.hpp"
size_t CountingAllocatorBase::construction_counter;
size_t CountingAllocatorBase::destruction_counter;
size_t CountingAllocatorBase::num_allocations;
size_t CountingAllocatorBase::num_frees;
#include <random>
#include <unordered_map>
#include <map>
#include <chrono>
#include <fstream>
namespace
{
template<typename T>
struct insert_performance
{
	T operator()(int amount, int seed, float /*load_factor*/) const __attribute__((noinline))
	{
		T hash_map;
		std::mt19937 engine(seed);
		std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
		for (int i = 0; i < amount; ++i)
		{
			hash_map[distribution(engine)] = i;
		}
		return hash_map;
	}
};
template<typename T>
struct insert_performance_reserve
{
	T operator()(int amount, int seed, float load_factor) const __attribute__((noinline))
	{
		T hash_map;
		hash_map.max_load_factor(load_factor);
		hash_map.reserve(amount);
		size_t bucket_count = hash_map.bucket_count();
		std::mt19937 engine(seed);
		std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
		for (int i = 0; i < amount; ++i)
		{
			hash_map[distribution(engine)] = i;
		}
		if (hash_map.bucket_count() != bucket_count)
		{
			throw std::runtime_error("the hash map reallocated, which isn't intended because I want to test for a certain load factor");
		}
		return hash_map;
	}
};

template<typename T>
struct modify_performance
{
	T operator()(int amount, int seed, float load_factor) const __attribute__((noinline))
	{
		T hash_map;
		hash_map.max_load_factor(load_factor);
		hash_map.reserve(amount);
		size_t bucket_count = hash_map.bucket_count();
		std::mt19937 engine(seed);
		std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
		std::vector<int> all_keys;
		for (int i = 0; i < amount; ++i)
		{
			all_keys.push_back(distribution(engine));
			hash_map[all_keys.back()] = i;
		}
		for (size_t i = 0; i < all_keys.size(); ++i)
		{
			auto found = hash_map.find(all_keys[i]);
			if (found != hash_map.end()) hash_map.erase(found);
			hash_map[distribution(engine)] = distribution(engine);
		}
		if (hash_map.bucket_count() != bucket_count)
		{
			throw std::runtime_error("the hash map reallocated, which isn't intended because I want to test for a certain load factor");
		}
		return hash_map;
	}
};
template<typename T>
struct erase_performance
{
	T operator()(int amount, int seed, float load_factor) const __attribute__((noinline))
	{
		T hash_map;
		hash_map.max_load_factor(load_factor);
		hash_map.reserve(amount);
		size_t bucket_count = hash_map.bucket_count();
		std::mt19937 engine(seed);
		std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
		std::vector<int> all_keys;
		for (int i = 0; i < amount; ++i)
		{
			all_keys.push_back(distribution(engine));
			hash_map[all_keys.back()] = i;
		}
		if (hash_map.bucket_count() != bucket_count)
		{
			throw std::runtime_error("the hash map reallocated, which isn't intended because I want to test for a certain load factor");
		}
		for (int key : all_keys)
		{
			hash_map.erase(key);
		}
		return hash_map;
	}
};
template<typename T>
struct performance_lookups
{
	T operator()(int amount, int seed, float load_factor) const __attribute__((noinline))
	{
		T hash_map;
		hash_map.max_load_factor(load_factor);
		hash_map.reserve(amount);
		size_t bucket_count = hash_map.bucket_count();
		std::mt19937 engine(seed);
		std::uniform_int_distribution<int> distribution(0, std::numeric_limits<int>::max());
		hash_map.reserve(amount);
		for (int i = 0; i < amount; ++i)
		{
			hash_map[distribution(engine)] = i;
		}
		if (hash_map.bucket_count() != bucket_count)
		{
			throw std::runtime_error("the hash map reallocated, which isn't intended because I want to test for a certain load factor");
		}
		std::vector<unsigned char> results;
		results.reserve(amount * 20);
		for (int i = 0; i < amount * 20; ++i)
		{
			results.push_back(hash_map.find(i) == hash_map.end());
		}
		return hash_map;
	}
};
constexpr int profile_amount = 10000;
struct LargeStruct
{
	LargeStruct()
		: a(0)
	{
	}
	LargeStruct(int a)
		: a(a)
	{
	}
	int a;
	char unused[508];
};
struct MediumStruct
{
	MediumStruct() = default;
	MediumStruct(int i)
		: i(i), a()
	{
	}
	int i;
	std::string a;
	bool operator==(const MediumStruct & other) const
	{
		return i == other.i && a == other.a;
	}
};
}
namespace std
{
template<>
struct hash<MediumStruct>
{
	size_t operator()(const MediumStruct & obj) const
	{
		return obj.i;
	}
};
}
namespace
{
struct Measurer
{
	Measurer()
		: before(std::chrono::high_resolution_clock::now())
	{
	}
	std::chrono::milliseconds duration()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - before);
	}

	std::chrono::high_resolution_clock::time_point before;
};

template<typename T>
void profile_single(std::ostream & out, const char * category, const char * name, int amount, int seed, float load_factor)
{
	Measurer measure;
	T()(amount, seed, load_factor);
	auto duration = measure.duration();
	out << category << " " << name << ": " << duration.count() << "\n";
}

template<template<typename> class Profile>
void profile(const char * name, int amount, int seed, float load_factor)
{
	std::ofstream out("performance_stats", std::ios_base::app);
	profile_single<Profile<std::unordered_map<int, int> > >(out, "unordered small", name, amount, seed, load_factor);
	profile_single<Profile<thin_sherwood_map<int, int> > >(out, "sherwood small", name, amount, seed, load_factor);
	profile_single<Profile<fat_sherwood_map<int, int> > >(out, "fat_sherwood small", name, amount, seed, load_factor);
	profile_single<Profile<std::unordered_map<MediumStruct, MediumStruct> > >(out, "unordered medium", name, amount, seed, load_factor);
	profile_single<Profile<thin_sherwood_map<MediumStruct, MediumStruct> > >(out, "sherwood medium", name, amount, seed, load_factor);
	profile_single<Profile<fat_sherwood_map<MediumStruct, MediumStruct> > >(out, "fat_sherwood medium", name, amount, seed, load_factor);
	profile_single<Profile<std::unordered_map<int, LargeStruct> > >(out, "unordered large", name, amount, seed, load_factor);
	profile_single<Profile<thin_sherwood_map<int, LargeStruct> > >(out, "sherwood large", name, amount, seed, load_factor);
	profile_single<Profile<fat_sherwood_map<int, LargeStruct> > >(out, "fat_sherwood large", name, amount, seed, load_factor);
}
constexpr int profile_repetition_count = 10;
static float profile_load_factor = thin_sherwood_map<int, int>().max_load_factor();
TEST(sherwood_map, DISABLED_profile_insertion)
{
	for (int i = 0; i < profile_repetition_count; ++i) profile<insert_performance>("insert", profile_amount, 5, profile_load_factor);
}
TEST(sherwood_map, profile_insertion_reserve)
{
	for (int i = 0; i < profile_repetition_count; ++i) profile<insert_performance_reserve>("insert_reserve", profile_amount, 5, profile_load_factor);
}
TEST(sherwood_map, DISABLED_profile_modify)
{
	for (int i = 0; i < profile_repetition_count; ++i) profile<modify_performance>("modify", profile_amount / 2, 6, profile_load_factor);
}
TEST(sherwood_map, DISABLED_profile_lookup)
{
	for (int i = 0; i < profile_repetition_count; ++i) profile<performance_lookups>("lookups", profile_amount / 5, 7, profile_load_factor);
}
TEST(sherwood_map, DISABLED_profile_erase)
{
	for (int i = 0; i < profile_repetition_count; ++i) profile<erase_performance>("erase", profile_amount / 2, 8, profile_load_factor);
}

TEST(sherwood_map, DISABLED_profile_first_insert)
{
	for (int i = 0; i < 1000000; ++i)
	{
		thin_sherwood_map<int, int> a;
		a.emplace(1, 1);
	}
}

#if 0
TEST(sherwood_map, performance)
{
	{
		std::map<float, double> load_factor_costs;
		for (float load_factor = 0.7f;; load_factor = std::min(1.0f, load_factor + 0.01f))
		{
			int limit = 1000;
			ScopedMeasurer measure("load_factor");
			size_t allocated_before = mem::MemoryManager::GetTotalBytesAllocated();
			double total_cost = 0.0;
			for (int starting_value = 0; starting_value < 3; starting_value += 2)
			{
				for (int i = 2; i < limit; i = i * 3 / 2)
				{
					total_cost += run_performance_test(i, starting_value, load_factor);
				}
			}
			load_factor_costs[load_factor] = total_cost;
			std::cout << load_factor << ": " << (mem::MemoryManager::GetTotalBytesAllocated() - allocated_before) << ", " << total_cost << ", ";
			if (load_factor == 1.0f)
				break;
		}
		std::map<size_t, float> sorted_load_factor_costs;
		for (const auto & pair : load_factor_costs)
		{
			sorted_load_factor_costs[pair.second] = pair.first;
		}
		for (const auto & pair : sorted_load_factor_costs)
		{
			std::cout << pair.first << ": " << pair.second << std::endl;
		}
	}
}
#endif
struct thin_sherwood_map_tester
{
	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> > >
	using map = thin_sherwood_map<K, V, H, E, A>;
};
/*struct fat_sherwood_map_tester
{
	template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V> > >
	using map = fat_sherwood_map<K, V, H, E, A>;
};*/
INSTANTIATE_TYPED_TEST_CASE_P(thin_sherwood_map, sherwood_test, thin_sherwood_map_tester);
//INSTANTIATE_TYPED_TEST_CASE_P(fat_sherwood_map, sherwood_test, fat_sherwood_map_tester);
}
#endif

