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

  unsigned int operator()(const Container & s,unsigned int n)const __attribute__((noinline))
  {
    unsigned int                                res=0;
    rand_seq                                    rnd(n);
    auto                                        end_=s.end();
    while(n--){
      if(s.find(rnd())!=end_)++res;
    }
    return res;
  }
};

template<typename Container>
struct scattered_unsuccessful_lookup
{
  typedef unsigned int result_type;

  unsigned int operator()(const Container & s,unsigned int n)const __attribute__((noinline))
  {
    unsigned int                                res=0;
    std::uniform_int_distribution<unsigned int> dist;
    std::mt19937                                gen(76453);
    auto                                        end_=s.end();
    while(n--){
      if(s.find(dist(gen))!=end_)++res;
    }
    return res;
  }
};

template<
  template<typename> class Tester,
  typename Container1,typename Container2,typename Container3, typename Container4>
static void test(std::ostream & out,
  const char* title,
  const char* name1,const char* name2,const char* name3, const char* name4)
{
  //unsigned int n0=10000,n1=3000000,dn=500;
	//unsigned int n0 = 13396, n1 = n0 + 1, dn = 20;
	unsigned int n0=10000,n1=3000000,dn=500;
  double       fdn=1.05;

  out<<title<<":"<<std::endl;
  out<<"amount;"<<name1<<";"<<name2<<";"<<name3<<";"<<name4<<std::endl;

  for(unsigned int n=n0;n<=n1;n+=dn,dn=(unsigned int)(dn*fdn)){
    double t;

	out << n;
	t=measure(boost::bind(
      Tester<Container1>(),
      boost::cref(create<Container1>(n)),n));
	out<<";"<<(t/n)*10E6;

    t=measure(boost::bind(
      Tester<Container2>(),
      boost::cref(create<Container2>(n)),n));
	out<<";"<<(t/n)*10E6;
 
	t=measure(boost::bind(
	  Tester<Container3>(),
	  boost::cref(create<Container3>(n)),n));
	out<<";"<<(t/n)*10E6;

	t=measure(boost::bind(
	  Tester<Container4>(),
	  boost::cref(create<Container4>(n)),n));
	out<<";"<<(t/n)*10E6<<std::endl;
  }
}

#include <boost/unordered_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <unordered_map>
#include "../finished/sherwood_map.hpp"
#include <gtest/gtest.h>
#include <fstream>

TEST(profile, lookup)
{
  using namespace boost::multi_index;

  /* some stdlibs provide the discussed but finally rejected std::identity */
  using boost::multi_index::member;

  typedef std::unordered_map<unsigned int, unsigned int>        container_t1;
  typedef boost::unordered_map<unsigned int, unsigned int>      container_t2;
  typedef boost::multi_index_container<
	std::pair<unsigned int, unsigned int>,
    indexed_by<
	  hashed_unique<member<std::pair<unsigned int, unsigned int>, unsigned int, &std::pair<unsigned int, unsigned int>::first> >
    >
  >                                               container_t3;
	typedef sherwood_map<unsigned int, unsigned int> container_t4;

	std::ofstream successful_out("successful_lookup");

  test<
    scattered_successful_lookup,
    container_t1,
    container_t2,
	container_t3, container_t4>
  (successful_out,
    "Scattered successful lookup",
	"std::unordered_map",
	"boost::unordered_map",
	"multi_index::hashed_unique",
			  "sherwood_map"
  );

  std::ofstream unsuccessful_out("unsuccessful_lookup");

  test<
    scattered_unsuccessful_lookup,
    container_t1,
    container_t2,
	container_t3, container_t4>
  (
			  unsuccessful_out,
    "Scattered unsuccessful lookup",
	"std::unordered_map",
	"boost::unordered_map",
	"multi_index::hashed_unique", "sherwood_map"
  );
}
