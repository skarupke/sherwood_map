#pragma once

#include <gtest/gtest.h>

struct IdentityHasher
{
	template<typename T>
	size_t operator()(T value) const
	{
		return value;
	}
};

template<typename T>
struct sherwood_test : ::testing::Test
{
};

TYPED_TEST_CASE_P(sherwood_test);

TYPED_TEST_P(sherwood_test, empty)
{
	typename TypeParam::template map<int, int> a;
	ASSERT_EQ(a.end(), a.find(5));
	ASSERT_TRUE(a.empty());
	a.emplace();
	a.clear();
	ASSERT_TRUE(a.empty());
}

TYPED_TEST_P(sherwood_test, simple)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a = { { 1, 5 }, { 2, 6 }, { 3, 7 } };
	ASSERT_EQ(3, a.size());
	a.insert(std::make_pair(1, 6));
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(5, a[1]);
	ASSERT_EQ(6, a[2]);
	ASSERT_EQ(7, a[3]);
	ASSERT_EQ(0, a[4]);
	ASSERT_EQ(4, a.size());
	auto found = a.find(3);
	ASSERT_NE(a.end(), found);
	ASSERT_EQ(3, found->first);
	ASSERT_EQ(7, found->second);
	ASSERT_EQ(a.end(), a.find(5));

	ASSERT_EQ((map_type{ { 1, 5 }, { 2, 6 }, { 3, 7 }, { 4, 0 } }), a);
}
TYPED_TEST_P(sherwood_test, conflicting_insert)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	a.reserve(5);
	a.insert({ { 1, 2 }, { 6, 5 }, { 11, 8 }, { 16, 11 } });
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(5, a[6]);
	ASSERT_EQ(8, a[11]);
	ASSERT_EQ(11, a[16]);
}

TYPED_TEST_P(sherwood_test, move_construct)
{
	typedef typename TypeParam::template map<std::string, std::unique_ptr<int> > map_type;
	map_type a;
	a["foo"] = std::unique_ptr<int>(new int(5));
	ASSERT_EQ(5, *a["foo"]);
	float load_factor = a.max_load_factor() / 2;
	a.max_load_factor(load_factor);
	std::pair<std::string, std::unique_ptr<int> > to_insert("foo", std::unique_ptr<int>(new int(6)));
	a.emplace(std::move(to_insert));
	map_type b(std::move(a));
	ASSERT_EQ(1, b.size());
	ASSERT_EQ(0, a.size());
	ASSERT_EQ(5, *b["foo"]);
	ASSERT_EQ(load_factor, b.max_load_factor());
}
TYPED_TEST_P(sherwood_test, copy)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a{ { 1, 2 }, { 3, 4 }, { 5, 6 } };
	a.max_load_factor(a.max_load_factor() / 2);
	map_type b(a);
	ASSERT_EQ(a, b);
	ASSERT_EQ(a.max_load_factor(), b.max_load_factor());
}
TYPED_TEST_P(sherwood_test, erase)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a = { { 1, 2 }, { 3, 4 } };
	ASSERT_EQ(2, a.size());
	ASSERT_EQ(1, a.erase(3));
	ASSERT_EQ(1, a.size());
	ASSERT_EQ((map_type{ { 1, 2 } }), a);
}
TYPED_TEST_P(sherwood_test, iterator_erase)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a = { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } };
	auto begin = a.erase(a.begin(), std::next(a.begin(), 2));
	ASSERT_EQ(a.begin(), begin);
	ASSERT_EQ(*a.begin(), *begin);
	begin = a.erase(a.begin());
	ASSERT_EQ(a.begin(), begin);
	ASSERT_EQ(*a.begin(), *begin);
	begin = a.erase(a.begin());
	ASSERT_EQ(a.end(), begin);
	ASSERT_TRUE(a.empty());
}
TYPED_TEST_P(sherwood_test, conflicting_iterator_erase)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 5;
	a.reserve(count);
	count = a.bucket_count();
	a.insert(
	{
		{ 0 * count + 1, 2 },
		{ 1 * count + 1, 5 },
		{ 2 * count + 1, 8 },
		{ 3 * count + 1, 11 }
	});
	auto begin = a.erase(a.begin(), std::next(a.begin(), 2));
	ASSERT_EQ(a.begin(), begin);
	ASSERT_EQ(*a.begin(), *begin);
	ASSERT_EQ((map_type{ { 2 * count + 1, 8 }, { 3 * count + 1, 11 } }), a);
	begin = a.erase(a.begin());
	ASSERT_EQ(a.begin(), begin);
	ASSERT_EQ(*a.begin(), *begin);
	ASSERT_EQ((map_type{ { 3 * count + 1, 11 } }), a);
	begin = a.erase(a.begin());
	ASSERT_EQ(a.end(), begin);
	ASSERT_TRUE(a.empty());
}
TYPED_TEST_P(sherwood_test, two_conflicting_iterator_erase)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 5;
	a.reserve(count);
	count = a.bucket_count();
	a.insert(
	{
		{ 0 * count + 1, 2 },
		{ 1 * count + 1, 5 },
		{ 1 * count - 1, 8 },
		{ 2 * count - 1, 11 },
		{ 3 * count - 1, 14 }
	});
	auto begin = a.erase(a.begin());
	ASSERT_EQ(a.begin(), begin);
	ASSERT_EQ(a.begin(), a.find(a.begin()->first));
	ASSERT_EQ((map_type{ { 0 * count + 1, 2 }, { 1 * count + 1, 5 }, { 1 * count - 1, 8 }, { 3 * count - 1, 14 } }), a);
}

TYPED_TEST_P(sherwood_test, conflicting_iterator_erase_middle)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 5;
	a.reserve(count);
	count = a.bucket_count();
	a.insert({ { 0 * count + 1, 2 }, { 1 * count + 1, 5 }, { 2 * count + 1, 8 }, { 3 * count + 1, 11 } });
	a.erase(std::next(a.begin()));
	ASSERT_EQ((map_type{ { 0 * count + 1, 2 }, { 2 * count + 1, 8 }, { 3 * count + 1, 11 } }), a);
}
TYPED_TEST_P(sherwood_test, erase_conflicting_with_non_conflicting)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 5;
	a.rehash(count);
	count = a.bucket_count();
	// the idea is this: you will end up with elements in this
	// order as they are here. then we erase the first two.
	// element 11 has to move over by two, element 3 only has to
	// move over by one
	a.insert(
	{
		{ 0 * count + 1, 2 },
		{ 1 * count + 1, 5 },
		{ 2 * count + 1, 8 },
		{ 0 * count + 3, 9 }
	});
	a.erase(a.begin(), std::next(a.begin(), 2));
	ASSERT_EQ((map_type{ { 0 * count + 3, 9 }, { 2 * count + 1, 8 } }), a);
}
TYPED_TEST_P(sherwood_test, range_erase)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a{ { 1, 5 }, { 2, 6 }, { 3, 7 }, { 4, 0 } };
	a.erase(std::next(a.begin()), a.end());
	ASSERT_EQ(1, a.size());
	a.erase(a.begin(), a.end());
	ASSERT_TRUE(a.empty());
}
TYPED_TEST_P(sherwood_test, erase_all)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	a.rehash(3);
	a.insert({ { 1, 5 }, { 2, 6 }, { 3, 7 } });
	ASSERT_EQ(a.end(), a.erase(a.begin(), a.end()));
	ASSERT_TRUE(a.empty());
	ASSERT_EQ(a.end(), a.begin());
}
TYPED_TEST_P(sherwood_test, erase_crowded)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 7;
	a.rehash(count);
	count = a.bucket_count();
	a[1 * count + 0] = 1;
	a[2 * count + 0] = 2;
	a[3 * count + 0] = 3;
	a[4 * count + 0] = 4;
	a[5 * count + 0] = 5;
	a[0 * count + 1] = 6;
	a[0 * count + 2] = 7;
	a.erase(std::next(a.begin(), 4), std::next(a.begin(), 6));
	ASSERT_EQ(1, a[1 * count + 0]);
	ASSERT_EQ(2, a[2 * count + 0]);
	ASSERT_EQ(3, a[3 * count + 0]);
	ASSERT_EQ(4, a[4 * count + 0]);
	ASSERT_EQ(7, a[0 * count + 2]);
}
TYPED_TEST_P(sherwood_test, erase_big)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 7;
	a.rehash(count);
	count = a.bucket_count();
	a[1 * count + 0] = 1;
	a[2 * count + 0] = 2;
	a[3 * count + 0] = 3;
	a[4 * count + 0] = 4;
	a[0 * count + 1] = 6;
	a[0 * count + 6] = 7;
	a.erase(std::next(a.begin(), 4), a.end());
	a[1 * count + 1] = 8;
	a[1 * count + 2] = 9;
	a[2 * count + 2] = 10;
	ASSERT_EQ(1, a[1 * count + 0]);
	ASSERT_EQ(2, a[2 * count + 0]);
	ASSERT_EQ(3, a[3 * count + 0]);
	ASSERT_EQ(4, a[4 * count + 0]);
	ASSERT_EQ(8, a[1 * count + 1]);
	ASSERT_EQ(9, a[1 * count + 2]);
	ASSERT_EQ(10, a[2 * count + 2]);
}
TYPED_TEST_P(sherwood_test, move_over_please)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 5;
	a.rehash(count);
	count = a.bucket_count();
	// the idea is this: 1 and 2 go to their spots. 5 goes to 0,
	// 10 can't go to 0 so goes to 1 and pushes 1 and 2 over,
	// 15 does the same thing
	a.insert(
	{
		{ 0 * count + 1, 5 },
		{ 0 * count + 2, 6 },
		{ 1 * count + 0, 7 },
		{ 2 * count + 0, 8 },
		{ 3 * count + 0, 9 }
	});
	ASSERT_EQ(5, a[0 * count + 1]);
	ASSERT_EQ(6, a[0 * count + 2]);
	ASSERT_EQ(7, a[1 * count + 0]);
	ASSERT_EQ(8, a[2 * count + 0]);
	ASSERT_EQ(9, a[3 * count + 0]);
}
TYPED_TEST_P(sherwood_test, move_over_distance)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 7;
	a.rehash(count);
	count = a.bucket_count();
	// the idea is this: 1 and 2 go to their spots. 7 goes to 0,
	// 14 can't go to 0 so goes to 1 and pushes 1 and 2 over,
	// now I've had a bug where the distance of element 2 was now
	// incorrect, so I place an object at 3 and it will push
	// 2 out of the way
	a.insert(
	{
		{ 0 * count + 1, 5 },
		{ 0 * count + 2, 6 },
		{ 1 * count + 0, 7 },
		{ 2 * count + 0, 8 },
		{ 3 * count + 0, 9 },
		{ 0 * count + 3, 10 }
	});
	ASSERT_EQ((map_type{ { 0 * count + 1, 5 }, { 0 * count + 2, 6 }, { 1 * count + 0, 7 }, { 2 * count + 0, 8 }, { 3 * count + 0, 9 }, { 0 * count + 3, 10 } }), a);
}
TYPED_TEST_P(sherwood_test, many_collisions)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 11;
	a.rehash(count);
	count = a.bucket_count();
	std::initializer_list<std::pair<int, int> > il =
	{
		{ 1 * count + 2, 8 },
		{ 0 * count + 2, 3 },
		{ 0 * count + 1, 2 },
		{ 0 * count + 5, 4 },
		{ 1 * count + 1, 6 },
		{ 2 * count + 1, 7 },
		{ 1 * count + 0, 10 },
		{ 2 * count + 0, 5 },
	};
	ASSERT_GE(a.bucket_count(), il.size());
	a.insert(il);
	for (auto it = il.begin(); it != il.end(); ++it)
	{
		auto found = a.find(it->first);
		ASSERT_NE(a.end(), found);
		ASSERT_EQ(*it, *found);
	}
}
TYPED_TEST_P(sherwood_test, more_collision_insert)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	size_t count = 11;
	a.rehash(count);
	count = a.bucket_count();
	std::initializer_list<std::pair<int, int> > il =
	{
		{ 1 * count + 1, 8 },
		{ 1 * count + 2, 6 },
		{ 2 * count + 1, 3 },
		{ 2 * count + 2, 7 },
		{ 1 * count + 3, 5 },
	};
	ASSERT_GE(a.bucket_count(), il.size());
	a.insert(il);
	for (auto it = il.begin(); it != il.end(); ++it)
	{
		auto found = a.find(it->first);
		ASSERT_NE(a.end(), found);
		ASSERT_EQ(*it, *found);
	}
}

TYPED_TEST_P(sherwood_test, emplace_hint)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a = { { 1, 2 }, { 3, 4 } };
	auto found = a.find(1);
	ASSERT_NE(a.end(), found);
	auto insert_point = a.erase(found);
	a.insert(insert_point, std::make_pair(1, 3));
	ASSERT_EQ((map_type{ { 1, 3 }, { 3, 4 } }), a);
}

TYPED_TEST_P(sherwood_test, special_value)
{
	size_t special_value = detail::empty_hash;
	typedef typename TypeParam::template map<size_t, int, IdentityHasher> map_type;
	map_type a = { { special_value, 1 }, { special_value + 1, 2 } };
	ASSERT_EQ(2, a.size());
	ASSERT_EQ(1, a[special_value]);
	ASSERT_EQ(2, a[special_value + 1]);
}
TYPED_TEST_P(sherwood_test, crowded_end)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	// test whether erase will correctly shift elements from the beginning
	// of the storage to the end of the storage when erasing near the end
	map_type a;
	size_t count = 31;
	a.rehash(count);
	count = a.bucket_count();
	a[1 * count - 3] = 5;
	a[2 * count - 3] = 6;
	a[3 * count - 3] = 7;
	a[4 * count - 3] = 8;
	a[5 * count - 3] = 9;
	a[6 * count - 3] = 10;
	a[7 * count - 3] = 11;
	ASSERT_EQ(count, a.bucket_count()); // reallocation would mess up my test
	ASSERT_EQ(4 * count - 3, a.begin()->first); // assume 28 goes to 28, 59 goes to 29, 90 goes to 30, 121 goes to 0
	ASSERT_EQ((map_type{ { 1 * count - 3, 5 }, { 2 * count - 3, 6 }, { 3 * count - 3, 7 }, { 4 * count - 3, 8 }, { 5 * count - 3, 9, }, { 6 * count - 3, 10 }, { 7 * count - 3, 11 } }), a);
	a.erase(1 * count - 3);
	ASSERT_EQ((map_type{ { 2 * count - 3, 6 }, { 3 * count - 3, 7 }, { 4 * count - 3, 8 }, { 5 * count - 3, 9, }, { 6 * count - 3, 10 }, { 7 * count - 3, 11 } }), a);
	// had a bug where elements remained and weren't markes as deleted properly
	ASSERT_EQ(a.size(), std::distance(a.begin(), a.end()));
	// had a bug where the stored distance of element 121 was incorrect. so insert and remove elements
	// to force it to shift to the wrong position
	a[1 * count - 4] = 12;
	a[2 * count - 4] = 13;
	a[3 * count - 4] = 14;
	a[4 * count - 4] = 15;
	a.erase(1 * count - 4);
	a.erase(2 * count - 4);
	a.erase(3 * count - 4);
	a.erase(4 * count - 4);
	ASSERT_EQ((map_type{ { 2 * count - 3, 6 }, { 3 * count - 3, 7 }, { 4 * count - 3, 8 }, { 5 * count - 3, 9, }, { 6 * count - 3, 10 }, { 7 * count - 3, 11 } }), a);
}
/*TYPED_TEST_P(sherwood_test, insert_another_test)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type map;
	std::initializer_list<std::pair<int, int> > values =
	{
		{ 1402653483 + 1287338067, 1 },
		{ 139681623 + 2139129078, 2 },
		{ 1015058721 + 1794934114, 3 },
		{ 123719013 + 24940015, 4 },
		{ 629625263 + 1739890741, 5 }
	};
	map.insert(values.begin(), values.end());
	for (auto it = values.begin(); it != values.end(); ++it)
	{
		ASSERT_EQ(it->second, map[it->first]);
	}
	ASSERT_EQ(values.size(), map.size());
}*/

TYPED_TEST_P(sherwood_test, swap)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a = { { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } };
	a.max_load_factor(0.5f);
	map_type b = { { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 } };
	b.max_load_factor(0.6f);
	a.swap(b);
	ASSERT_EQ((map_type{ { 1, 1 }, { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 } }), b);
	ASSERT_EQ(5, b.size());
	ASSERT_EQ(0.5f, b.max_load_factor());
	ASSERT_EQ((map_type{ { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 7, 7 } }), a);
	ASSERT_EQ(6, a.size());
	ASSERT_EQ(0.6f, a.max_load_factor());
}
struct CtorDtorCounter
{
	CtorDtorCounter(int & ctor_counter, int & dtor_counter)
		: ctor_counter(ctor_counter), dtor_counter(dtor_counter)
	{
		++ctor_counter;
	}
	CtorDtorCounter(const CtorDtorCounter & other)
		: ctor_counter(other.ctor_counter), dtor_counter(other.dtor_counter)
	{
		++ctor_counter;
	}
	CtorDtorCounter & operator=(const CtorDtorCounter &)
	{
		return *this;
	}
	~CtorDtorCounter()
	{
		++dtor_counter;
	}

private:
	int & ctor_counter;
	int & dtor_counter;
};
struct CountingAllocatorBase
{
	static size_t construction_counter;
	static size_t destruction_counter;
	static size_t num_allocations;
	static size_t num_frees;
};
template<typename T>
struct CountingAllocator : CountingAllocatorBase
{
	CountingAllocator() = default;
	template<typename U>
	CountingAllocator(CountingAllocator<U>)
	{
	}

	typedef T value_type;
	template<typename U>
	struct rebind
	{
		typedef CountingAllocator<U> other;
	};

	T * allocate(size_t n, const void * = nullptr)
	{
		++num_allocations;
		return static_cast<T *>(malloc(sizeof(T) * n));
	}
	void deallocate(T * ptr, size_t)
	{
		++num_frees;
		return free(ptr);
	}
	template<typename U, typename... Args>
	void construct(U * ptr, Args &&... args)
	{
		new (ptr) U(std::forward<Args>(args)...);
		++construction_counter;
	}
	template<typename U>
	void destroy(U * ptr)
	{
		++destruction_counter;
		ptr->~U();
	}
};
struct ScopedAssertNoLeaks
{
	ScopedAssertNoLeaks(bool expect_allocations, bool expect_constructions)
		: expect_allocations(expect_allocations)
		, expect_constructions(expect_constructions)
		, constructions_before(CountingAllocatorBase::construction_counter)
		, destructions_before(CountingAllocatorBase::destruction_counter)
		, allocations_before(CountingAllocatorBase::num_allocations)
		, frees_before(CountingAllocatorBase::num_frees)
	{
	}
	~ScopedAssertNoLeaks()
	{
		run_asserts();
	}

private:
	bool expect_allocations;
	bool expect_constructions;
	size_t constructions_before;
	size_t destructions_before;
	size_t allocations_before;
	size_t frees_before;

	void run_asserts()
	{
		if (expect_allocations) ASSERT_NE(CountingAllocatorBase::num_allocations, allocations_before);
		else ASSERT_EQ(CountingAllocatorBase::num_allocations, allocations_before);
		if (expect_constructions) ASSERT_NE(CountingAllocatorBase::construction_counter, constructions_before);
		else ASSERT_EQ(CountingAllocatorBase::construction_counter, constructions_before);
		ASSERT_EQ(CountingAllocatorBase::construction_counter - constructions_before, CountingAllocatorBase::destruction_counter - destructions_before);
		ASSERT_EQ(CountingAllocatorBase::num_allocations - allocations_before, CountingAllocatorBase::num_frees - frees_before);
	}
};
TYPED_TEST_P(sherwood_test, destructor_caller)
{
	typedef typename TypeParam::template map<int, CtorDtorCounter, std::hash<int>, std::equal_to<int>, CountingAllocator<std::pair<int, CtorDtorCounter> > > map_type;
	ScopedAssertNoLeaks leak_check(true, true);
	int ctor_counter = 0;
	int dtor_counter = 0;
	{
		map_type a;
		for (int i = 0; i < 4; ++i)
		{
			a.emplace(i, CtorDtorCounter(ctor_counter, dtor_counter));
		}
		a.erase(5);
		a.erase(a.begin(), std::next(a.begin(), 3));
	}
	ASSERT_NE(0, ctor_counter);
	ASSERT_EQ(ctor_counter, dtor_counter);
}
struct throwing_allocator_exception {};
template<typename T>
struct throwing_allocator : CountingAllocator<T>
{
	throwing_allocator() = default;
	template<typename U>
	throwing_allocator(const throwing_allocator<U> &)
	{
	}

	template<typename U>
	struct rebind
	{
		typedef throwing_allocator<U> other;
	};

	template<typename U, typename... Args>
	void construct(U *, Args &&...)
	{
		throw throwing_allocator_exception{};
	}
};

TYPED_TEST_P(sherwood_test, throwing_allocator_test)
{
	ScopedAssertNoLeaks leak_check(true, false);
	auto construct = []{ typename TypeParam::template map<int, int, std::hash<int>, std::equal_to<int>, throwing_allocator<std::pair<const int, int> > >{ { 1, 2 }, { 2, 3 } }; };
	ASSERT_THROW(construct(), throwing_allocator_exception);
}

struct stateful_hasher
{
	stateful_hasher() = default;
	stateful_hasher(const stateful_hasher & other)
		: to_add(other.to_add + 1)
	{
	}
	stateful_hasher(stateful_hasher &&) = default;
	stateful_hasher & operator=(const stateful_hasher & other)
	{
		to_add = other.to_add + 1;
		return *this;
	}
	stateful_hasher & operator=(stateful_hasher &&) = default;

	template<typename T>
	size_t operator()(const T & value) const
	{
		return std::hash<T>()(value) + to_add;
	}

	size_t to_add = 0;
};

TYPED_TEST_P(sherwood_test, stateful_hasher_test)
{
	typedef typename TypeParam::template map<int, int, stateful_hasher> map_type;
	map_type a{ { 1, 2 }, { 3, 4 }, { 5, 7 }, { 8, 9 } };
	a = a;
	ASSERT_EQ((map_type{ { 1, 2 }, { 3, 4 }, { 5, 7 }, { 8, 9 } }), a);
}

TYPED_TEST_P(sherwood_test, load_factor)
{
	typename TypeParam::template map<int, int> a;
	ASSERT_EQ(0, a.load_factor());
}
TYPED_TEST_P(sherwood_test, max_load_factor)
{
	typedef typename TypeParam::template map<int, int> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	a.insert({ { 1, 2 }, { 2, 3 }, { 3, 4 } });
	ASSERT_EQ((map_type{ { 1, 2 }, { 2, 3 }, { 3, 4 } }), a);
}
TYPED_TEST_P(sherwood_test, dont_grow_when_you_wont_insert)
{
	typename TypeParam::template map<int, int> a;
	a.max_load_factor(1.0f);
	a.reserve(3);
	size_t bucket_count_before = a.bucket_count();
	a.insert({ { 1, 2 }, { 3, 4 }, { 5, 6 }, { 5, 6 } });
	ASSERT_EQ(bucket_count_before, a.bucket_count());
}
TYPED_TEST_P(sherwood_test, allow_growing_with_max_load_factor_1)
{
	typedef typename TypeParam::template map<int, int, IdentityHasher> map_type;
	map_type a;
	a.max_load_factor(1.0f);
	a.reserve(3);
	a.insert({ { 1, 2 }, { 4, 4 }, { 7, 6 }, { 10, 8 } });
	ASSERT_EQ((map_type{ { 1, 2 }, { 4, 4 }, { 7, 6 }, { 10, 8 } }), a);
}

REGISTER_TYPED_TEST_CASE_P(sherwood_test, empty, simple, move_construct, copy, erase, iterator_erase, conflicting_iterator_erase, range_erase, move_over_please, emplace_hint, special_value, crowded_end, swap, destructor_caller, throwing_allocator_test, stateful_hasher_test, load_factor, max_load_factor, dont_grow_when_you_wont_insert, allow_growing_with_max_load_factor_1, erase_conflicting_with_non_conflicting, conflicting_insert, move_over_distance, many_collisions, more_collision_insert, conflicting_iterator_erase_middle, two_conflicting_iterator_erase, erase_all, erase_crowded, erase_big);
