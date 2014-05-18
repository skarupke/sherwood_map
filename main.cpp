#include <gtest/gtest.h>
#include "sherwood_map.hpp"
#include <unordered_map>

#define CONCAT2(a, b) a ## b
#define CONCAT(a, b) CONCAT2(a, b)

#define USE_STRUCTS_2(i)\
USE_A_STRUCT(i);\
USE_A_STRUCT(CONCAT(i, 1))
#define USE_STRUCTS_4(i)\
USE_STRUCTS_2(i);\
USE_STRUCTS_2(CONCAT(i, 2))
#define USE_STRUCTS_8(i)\
USE_STRUCTS_4(i);\
USE_STRUCTS_4(CONCAT(i, 3))
#define USE_STRUCTS_16(i)\
USE_STRUCTS_8(i);\
USE_STRUCTS_8(CONCAT(i, 4))
#define USE_STRUCTS_32(i)\
USE_STRUCTS_16(i);\
USE_STRUCTS_16(CONCAT(i, 5))
#define USE_STRUCTS_64(i)\
USE_STRUCTS_32(i);\
USE_STRUCTS_32(CONCAT(i, 6))
#define USE_STRUCTS_128(i)\
USE_STRUCTS_64(i);\
USE_STRUCTS_64(CONCAT(i, 7))
#define USE_STRUCTS_256(i)\
USE_STRUCTS_128(i);\
USE_STRUCTS_128(CONCAT(i, 8))
#define USE_STRUCTS_512(i)\
USE_STRUCTS_256(i);\
USE_STRUCTS_256(CONCAT(i, 9))
#define USE_STRUCTS_1024(i)\
USE_STRUCTS_512(i);\
USE_STRUCTS_512(CONCAT(i, 0))

#define INSTANTIATE2(name) name(0)
#define INSTANTIATE(i) INSTANTIATE2(CONCAT(USE_STRUCTS_, i))

#ifndef NUM_ITERATIONS
#	define NUM_ITERATIONS 64
#endif

#define COMPILE_HASH_MAP
#define UNORDERED_HASH_MAP
#ifdef COMPILE_HASH_MAP
#	ifdef UNORDERED_HASH_MAP
#		include <unordered_map>
template<typename K, typename V>
using map_type = std::unordered_map<K, V>;
#	else
#		include "sherwood_map.hpp"
template<typename K, typename V>
using map_type = fat_sherwood_map<K, V>;
#	endif
#	define USE_A_STRUCT(i)\
struct CONCAT(A, i)\
{\
};\
map_type<int, CONCAT(A, i)> CONCAT(foo, i)()\
{\
	map_type<int, CONCAT(A, i)> map;\
	map.emplace();\
	map.erase(0);\
	return map;\
}\
map_type<int, CONCAT(A, i)> CONCAT(static, i) = CONCAT(foo, i)()
INSTANTIATE(NUM_ITERATIONS);
#endif



/*template<typename T>
T TestMemoryUsage(int amount)
{
	T hash_map;
	hash_map.reserve(amount);
	for (int i = 0; i < amount; ++i) hash_map.emplace(i, "");
	return hash_map;
}*/

int main(int argc, char * argv[])
{
	//TestMemoryUsage<fat_sherwood_map<int, std::string>>(1000000);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

