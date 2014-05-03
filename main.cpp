#include <gtest/gtest.h>
#include "sherwood_map.hpp"
#include <unordered_map>

template<typename T>
T TestMemoryUsage(int amount)
{
	T hash_map;
	hash_map.reserve(amount);
	for (int i = 0; i < amount; ++i) hash_map.emplace(i, "");
	return hash_map;
}

int main(int argc, char * argv[])
{
	TestMemoryUsage<fat_sherwood_map<int, std::string>>(1000000);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

