/* Measuring running insertion times of unordered associative containers
 * without duplicate elements.
 *
 * Copyright 2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "profile_shared.h"

template<typename Container>
struct running_insertion
{
  typedef unsigned int result_type;

	running_insertion(unsigned int n)
		: amount(n)
	{
	}

  unsigned int operator()()const __attribute__((noinline))
  {
    unsigned int res=0;
    {
      Container s;
	  rand_seq  rnd(amount);
	  for (unsigned int n = amount; n--; )insert_random(s, rnd);
      res=s.size();
      pause_timing();
    }
    resume_timing();
    return res;
  }
private:
  unsigned int amount;
};

template<typename Container>
struct norehash_running_insertion
{
  typedef unsigned int result_type;
	norehash_running_insertion(unsigned int n)
		: amount(n)
	{
	}

  unsigned int operator()()const __attribute__((noinline))
  {
    unsigned int res=0;
    {
      Container s;
	  rand_seq  rnd(amount);
	  reserve(s,amount);
	  for (unsigned int n = amount; n--; )
		  insert_random(s, rnd);
      res=s.size();
      pause_timing();
    }
    resume_timing();
    return res;
  }
private:
  unsigned int amount;
};

#include <gtest/gtest.h>

TEST(profile, DISABLED_insertion)
{
	test<norehash_running_insertion>("unique_no_rehash_running_insertion", "No-rehash runnning insertion");
	test<running_insertion>("unique_running_insertion", "Runnning insertion");
}
