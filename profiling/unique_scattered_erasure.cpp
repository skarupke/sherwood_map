/* Measuring erasure times of unordered associative containers
 * without duplicate elements.
 *
 * Copyright 2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "profile_shared.h"

template<typename Container>
struct scattered_erasure
{
  typedef unsigned int result_type;

	scattered_erasure(unsigned int n)
		: amount(n)
	{
	}

  unsigned int operator()()const __attribute__((noinline))
  {
    unsigned int res=0;
    {
      pause_timing();
      Container s;
	  rand_seq  rnd(amount);
	  reserve(s, amount);
	  for (unsigned int n = amount; n--;)insert_random(s, rnd);
      res=s.size();
      std::vector<typename Container::iterator> v;
      v.reserve(s.size());
      for(auto it=s.begin();it!=s.end();++it)v.push_back(it);
      std::mt19937 gen(73642);
      std::shuffle(v.begin(),v.end(),gen);
      resume_timing();
      for(auto it:v)s.erase(it);
      pause_timing();
    }
    resume_timing();
    return res;
  }

private:
  unsigned int amount;
};

#include <fstream>
#include <gtest/gtest.h>

TEST(profile, DISABLED_erase)
{
	test<scattered_erasure>("erase", "Scattered erasure");
}
