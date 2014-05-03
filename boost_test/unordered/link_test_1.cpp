
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "../helpers/postfix.hpp"
#include <gtest/gtest.h>
#include "sherwood_map.hpp"

void foo(boost::unordered_set<int>&,
		sherwood_map<int, int>&,
		fat_sherwood_map<int, int>&,
        boost::unordered_multiset<int>&,
        boost::unordered_multimap<int, int>&);

TEST(boost_tests, link_test_1)
{
    boost::unordered_set<int> x1;
	sherwood_map<int, int> x2;
	fat_sherwood_map<int, int> fx2;
	boost::unordered_multiset<int> x3;
    boost::unordered_multimap<int, int> x4;

	foo(x1, x2, fx2, x3, x4);
}
