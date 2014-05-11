#pragma once

// the two array version stores hashes in one array, and key/value pairs
// in another array. in theory this allows faster finding of nodes in case
// of hash collisions
#include "finished/sherwood_map.hpp"
// the one array version stores hash, key and value in the same array
#include "sherwood_map_one_array.hpp"

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>,typename A = std::allocator<std::pair<K, V> > >
using fat_sherwood_map = sherwood_map<K, V, H, E, A>;
