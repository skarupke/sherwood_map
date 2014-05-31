#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>

#include <boost/unordered_map.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <unordered_map>
#include "sherwood_map.hpp"
#include <fstream>

inline std::chrono::high_resolution_clock::time_point & measure_start()
{
	static std::chrono::high_resolution_clock::time_point result;
	return result;
}
inline std::chrono::high_resolution_clock::time_point & measure_pause()
{
	static std::chrono::high_resolution_clock::time_point result;
	return result;
}

template<typename F>
double measure(F f)
{
  using namespace std::chrono;

  static const int              num_trials=10;
  static const milliseconds     min_time_per_trial(200);
  std::array<double,num_trials> trials;

  for(int i=0;i<num_trials;++i){
	int                               runs=0;
	high_resolution_clock::time_point t2;

	measure_start()=high_resolution_clock::now();
	do{
	  f();
	  ++runs;
	  t2=high_resolution_clock::now();
	}while(t2-measure_start()<min_time_per_trial);
	trials[i]=duration_cast<duration<double>>(t2-measure_start()).count()/runs;
  }

  std::sort(trials.begin(),trials.end());
  return std::accumulate(
	trials.begin()+2,trials.end()-2,0.0)/(trials.size()-4);
}

inline void pause_timing()
{
  measure_pause()=std::chrono::high_resolution_clock::now();
}

inline void resume_timing()
{
  measure_start()+=std::chrono::high_resolution_clock::now()-measure_pause();
}

#include <boost/bind.hpp>
#include <iostream>
#include <random>
#include <boost/multi_index_container.hpp>

struct rand_seq
{
  rand_seq(unsigned int):gen(34862){}
  unsigned int operator()()
  {
	  return dist(gen) * 16;
  }

private:
  std::uniform_int_distribution<unsigned int> dist;
  std::mt19937                                gen;
};

template<typename Container>
void reserve(Container& s,unsigned int n)
{
	s.reserve(n);
}
template<typename V, typename I, typename A>
void reserve(boost::multi_index::multi_index_container<V, I, A> & s, unsigned int n)
{
	s.rehash(std::ceil(n / s.max_load_factor()));
}

template<typename T>
inline void insert_random(T & container, rand_seq & rnd)
{
	unsigned int to_insert = rnd();
	container.insert(std::make_pair(to_insert, to_insert));
}

template<typename>
struct name_for;
template<typename K, typename V, typename H, typename E, typename A>
struct name_for<std::unordered_map<K, V, H, E, A> >
{
	const char * operator()() const
	{
		return "std::unordered_map";
	}
};
template<typename K, typename V, typename H, typename E, typename A>
struct name_for<boost::unordered_map<K, V, H, E, A> >
{
	const char * operator()() const
	{
		return "boost::unordered_map";
	}
};
template<typename V, typename I, typename A>
struct name_for<boost::multi_index_container<V, I, A> >
{
	const char * operator()() const
	{
		return "boost::multi_index";
	}
};
template<typename K, typename V, typename H, typename E, typename A>
struct name_for<sherwood_map<K, V, H, E, A> >
{
	const char * operator()() const
	{
		return "sherwood_map";
	}
};
template<typename K, typename V, typename H, typename E, typename A>
struct name_for<fat_sherwood_map<K, V, H, E, A> >
{
	const char * operator()() const
	{
		return "fat_sherwood_map";
	}
};

template<typename Container, typename... More>
struct container_printer
{
	void operator()(std::ostream & out)
	{
		container_printer<Container>()(out);
		container_printer<More...>()(out);
	}
};
template<typename Container>
struct container_printer<Container>
{
	void operator()(std::ostream & out)
	{
		out << ";" << name_for<Container>()();
	}
};

template<template<typename> class Tester, typename Container, typename... More>
struct container_measurer
{
	void operator()(std::ostream & out, unsigned int n, unsigned int reference)
	{
		container_measurer<Tester, Container>()(out, n, reference);
		container_measurer<Tester, More...>()(out, n, reference);
	}
};
template<template<typename> class Tester, typename Container>
struct container_measurer<Tester, Container>
{
	void operator()(std::ostream & out, unsigned int n, unsigned int reference)
	{
		double t = measure(Tester<Container>(n));
		out << ";" << (t / reference) * 10E6;
	}
};

template<template<typename> class Tester, typename Container, typename... More>
void test_containers(std::ostream & out, const char* title)
{
	unsigned int n0=10000,n1=3000000,dn=500;
	//unsigned int n0=10500,n1=n0+1,dn=500;
	double       fdn=1.05;

  out<<title<<":"<<std::endl;
  out<<"amount";
  container_printer<Container, More...>()(out);
  out<<std::endl;

  for(unsigned int n=n0;n<=n1;n+=dn,dn=(unsigned int)(dn*fdn))
  {
	unsigned int m = Tester<Container>(n)();
	out << n;
	container_measurer<Tester, Container, More...>()(out, n, m);
	out<<std::endl;
  }
}


template<template<typename> class Tester, typename T>
void test_typed(std::ostream & out, const char * title)
{
	using namespace boost::multi_index;
	using boost::multi_index::member;

	typedef std::unordered_map<unsigned int, T> container_t1;
	typedef boost::unordered_map<unsigned int, T> container_t2;
	typedef boost::multi_index_container<std::pair<unsigned int, T>, indexed_by<hashed_unique<member<std::pair<unsigned int, T>, unsigned int, &std::pair<unsigned int, T>::first > > > > container_t3;
	typedef sherwood_map<unsigned int, T> container_t4;
	typedef thin_sherwood_map<unsigned int, T> container_t5;

	test_containers<Tester, container_t1, container_t2, container_t3, container_t4, container_t5>(out, title);
}

struct medium_struct
{
	medium_struct(unsigned int value)
		: value(value)
	{
	}

	unsigned int value;
	std::string a_string;
	std::string another_string;
};
struct large_struct
{
	large_struct(unsigned int value)
		: value(value)
	{
	}

	unsigned int value;
	unsigned char bytes[1024];
};

template<template<typename> class Tester>
void test(std::string filename, const char * title)
{
	std::ofstream out_small(filename + "_small");
	test_typed<Tester, unsigned int>(out_small, title);
	std::ofstream out_medium(filename + "_medium");
	test_typed<Tester, medium_struct>(out_medium, title);
	std::ofstream out_large(filename + "_large");
	test_typed<Tester, large_struct>(out_large, title);
}
