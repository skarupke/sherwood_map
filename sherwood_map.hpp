#pragma once

// the one array version stores hash, key and value in the same array
#include "sherwood_map_one_array.hpp"
// the two array version stores hashes in one array, and key/value pairs
// in another array. in theory this allows faster finding of nodes in case
// of hash collisions
#include "sherwood_map_two_arrays.hpp"
