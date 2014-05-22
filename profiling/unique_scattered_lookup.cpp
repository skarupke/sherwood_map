/* Measuring lookup times of unordered associative containers
 * without duplicate elements.
 *
 * Copyright 2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include "profile_shared.h"

template<typename Container>
Container create(unsigned int n)
{
  Container s;
  rand_seq  rnd(n);
  while(n--)insert_random(s, rnd);
  return s;
}

template<typename Container>
struct scattered_successful_lookup
{
  typedef unsigned int result_type;

	scattered_successful_lookup(unsigned int n)
		: container(create<Container>(n)), amount(n)
	{
	}

  unsigned int operator()()const __attribute__((noinline))
  {
    unsigned int                                res=0;
	rand_seq                                    rnd(amount);
	auto                                        end_=container.end();
	unsigned int n = amount;
	while(n--){
	  if(container.find(rnd())!=end_)++res;
    }
    return res;
  }
private:
  Container container;
  unsigned int amount;
};

template<typename Container>
struct scattered_unsuccessful_lookup
{
  typedef unsigned int result_type;

	scattered_unsuccessful_lookup(unsigned int n)
		: container(create<Container>(n)), amount(n)
	{
	}

  unsigned int operator()()const __attribute__((noinline))
  {
    unsigned int                                res=0;
    std::uniform_int_distribution<unsigned int> dist;
    std::mt19937                                gen(76453);
	auto                                        end_=container.end();
	unsigned int n = amount;
	while(n--)
	{
	  if(container.find(dist(gen))!=end_)++res;
    }
	return amount - res;
  }

private:
  Container container;
  unsigned int amount;
};

#include <gtest/gtest.h>
#include <fstream>

TEST(profile, DISABLED_lookup)
{
	test<scattered_successful_lookup>("successful_lookup", "Scattered successful lookup");
	test<scattered_unsuccessful_lookup>("unsuccessful_lookup", "Scattered unsuccessful lookup");
}
