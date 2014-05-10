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

  unsigned int operator()(unsigned int n)const
  {
    unsigned int res=0;
    {
      pause_timing();
      Container s;
      rand_seq  rnd(n);
	  while(n--)insert_random(s, rnd);
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
};

template<
  template<typename> class Tester,
  typename Container1,typename Container2,typename Container3, typename Container4>
static void test(std::ostream & out,
  const char* title,
  const char* name1,const char* name2,const char* name3, const char* name4)
{
  unsigned int n0=10000,n1=3000000,dn=500;
  double       fdn=1.05;

  out<<title<<":"<<std::endl;
  out<<name1<<";"<<name2<<";"<<name3<<";"<<name4<<std::endl;

  for(unsigned int n=n0;n<=n1;n+=dn,dn=(unsigned int)(dn*fdn)){
    double t;
    unsigned int m=Tester<Container1>()(n);

    t=measure(boost::bind(Tester<Container1>(),n));
	out<<m<<";"<<(t/m)*10E6;

    t=measure(boost::bind(Tester<Container2>(),n));
	out<<";"<<(t/m)*10E6;
 
	t=measure(boost::bind(Tester<Container3>(),n));
	out<<";"<<(t/m)*10E6;

	t=measure(boost::bind(Tester<Container4>(),n));
	out<<";"<<(t/m)*10E6<<std::endl;
  }
}

#include <boost/unordered_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <unordered_map>
#include "../finished/sherwood_map.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(profile, DISABLED_erase)
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

	std::ofstream out("erase");

  test<
    scattered_erasure,
    container_t1,
    container_t2,
	container_t3, container_t4>
  (
			  out,
    "Scattered erasure",
	"std::unordered_map",
	"boost::unordered_map",
	"multi_index::hashed_unique",
	"sherwood_map"
  );
}
