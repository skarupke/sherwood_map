#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <numeric>

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
	  return dist(gen);
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

