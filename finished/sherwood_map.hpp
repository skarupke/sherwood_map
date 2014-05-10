/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#pragma once

#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <stdexcept>

namespace detail
{
size_t next_prime(size_t size);
template<typename Result, typename Functor>
struct functor_storage : Functor
{
	functor_storage() = default;
	functor_storage(const Functor & functor)
		: Functor(functor)
	{
	}
	template<typename... Args>
	Result operator()(Args &&... args)
	{
		return static_cast<Functor &>(*this)(std::forward<Args>(args)...);
	}
	template<typename... Args>
	Result operator()(Args &&... args) const
	{
		return static_cast<const Functor &>(*this)(std::forward<Args>(args)...);
	}
};
template<typename Result, typename... Args>
struct functor_storage<Result, Result (*)(Args...)>
{
	typedef Result (*function_ptr)(Args...);
	function_ptr function;
	functor_storage(function_ptr function)
		: function(function)
	{
	}
	Result operator()(Args... args) const
	{
		return function(std::forward<Args>(args)...);
	}
	operator function_ptr &()
	{
		return function;
	}
	operator const function_ptr &()
	{
		return function;
	}
};
constexpr size_t empty_hash = 0;
inline size_t adjust_for_empty_hash(size_t value)
{
	return std::max(size_t(1), value);
}
template<typename key_type, typename value_type, typename hasher>
struct KeyOrValueHasher : functor_storage<size_t, hasher>
{
	typedef functor_storage<size_t, hasher> hasher_storage;
	KeyOrValueHasher(const hasher & hash)
		: hasher_storage(hash)
	{
	}
	size_t operator()(const key_type & key)
	{
		return adjust_for_empty_hash(static_cast<hasher_storage &>(*this)(key));
	}
	size_t operator()(const key_type & key) const
	{
		return adjust_for_empty_hash(static_cast<const hasher_storage &>(*this)(key));
	}
	size_t operator()(const value_type & value)
	{
		return adjust_for_empty_hash(static_cast<hasher_storage &>(*this)(value.first));
	}
	size_t operator()(const value_type & value) const
	{
		return adjust_for_empty_hash(static_cast<const hasher_storage &>(*this)(value.first));
	}
	template<typename F, typename S>
	size_t operator()(const std::pair<F, S> & value)
	{
		return adjust_for_empty_hash(static_cast<hasher_storage &>(*this)(value.first));
	}
	template<typename F, typename S>
	size_t operator()(const std::pair<F, S> & value) const
	{
		return adjust_for_empty_hash(static_cast<const hasher_storage &>(*this)(value.first));
	}
};
template<typename key_type, typename value_type, typename key_equal>
struct KeyOrValueEquality : functor_storage<bool, key_equal>
{
	typedef functor_storage<bool, key_equal> equality_storage;
	KeyOrValueEquality(const key_equal & equality)
		: equality_storage(equality)
	{
	}
	bool operator()(const key_type & lhs, const key_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs, rhs);
	}
	bool operator()(const key_type & lhs, const value_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs, rhs.first);
	}
	bool operator()(const value_type & lhs, const key_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs);
	}
	bool operator()(const value_type & lhs, const value_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
	}
	template<typename F, typename S>
	bool operator()(const key_type & lhs, const std::pair<F, S> & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs, rhs.first);
	}
	template<typename F, typename S>
	bool operator()(const std::pair<F, S> & lhs, const key_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs);
	}
	template<typename F, typename S>
	bool operator()(const value_type & lhs, const std::pair<F, S> & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
	}
	template<typename F, typename S>
	bool operator()(const std::pair<F, S> & lhs, const value_type & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
	}
	template<typename FL, typename SL, typename FR, typename SR>
	bool operator()(const std::pair<FL, SL> & lhs, const std::pair<FR, SR> & rhs)
	{
		return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
	}
};
template<typename T>
struct lazily_defauly_construct
{
	operator T() const
	{
		return T();
	}
};
template<typename It>
struct WrappingIterator : std::iterator<std::forward_iterator_tag, void, ptrdiff_t, void, void>
{
	WrappingIterator(It it, It begin, It end)
		: it(it), begin(begin), end(end)
	{
	}
	WrappingIterator & operator++()
	{
		if (++it == end)
			it = begin;
		return *this;
	}
	WrappingIterator operator++(int)
	{
		WrappingIterator copy(*this);
		++*this;
		return copy;
	}
	bool operator==(const WrappingIterator & other) const
	{
		return it == other.it;
	}
	bool operator!=(const WrappingIterator & other) const
	{
		return !(*this == other);
	}

	It it;
	It begin;
	It end;
};
inline size_t required_capacity(size_t size, float load_factor)
{
	return std::ceil(size / load_factor);
}
std::invalid_argument invalid_max_load_factor();
std::logic_error invalid_code_in_emplace();
std::out_of_range at_out_of_range();
}

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equality = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<Key, Value> > >
class sherwood_map
{
public:
	typedef Key key_type;
	typedef Value mapped_type;
	typedef std::pair<Key, Value> value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef Hash hasher;
	typedef Equality key_equal;
	typedef Allocator allocator_type;
	typedef value_type & reference;
	typedef const value_type & const_reference;
private:
	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<value_type > value_alloc;
	typedef std::allocator_traits<value_alloc> value_allocator_traits;
	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<size_t> hash_alloc;
	typedef std::allocator_traits<hash_alloc> hash_allocator_traits;
	typedef typename value_allocator_traits::pointer value_pointer;
	typedef typename value_allocator_traits::const_pointer const_value_pointer;
	typedef typename hash_allocator_traits::pointer hash_pointer;
	typedef typename hash_allocator_traits::const_pointer const_hash_pointer;
public:
	typedef typename value_allocator_traits::pointer pointer;
	typedef typename value_allocator_traits::const_pointer const_pointer;

private:
	struct StorageType;

	typedef detail::KeyOrValueHasher<key_type, value_type, hasher> KeyOrValueHasher;
	typedef detail::KeyOrValueEquality<key_type, value_type, key_equal> KeyOrValueEquality;

	template<typename O, typename HashIt, typename ValueIt>
	struct templated_iterator : std::iterator<std::forward_iterator_tag, O, ptrdiff_t>
	{
		templated_iterator()
			: hash_it(), hash_end(), value_it()
		{
		}
		templated_iterator(HashIt hash_it, HashIt hash_end, ValueIt value_it)
			: hash_it(hash_it), hash_end(hash_end), value_it(value_it)
		{
			while (this->hash_it != this->hash_end && *this->hash_it == detail::empty_hash)
			{
				++this->hash_it;
				++this->value_it;
			}
		}
		template<typename OO, typename OHashIt, typename OValueIt>
		templated_iterator (templated_iterator<OO, OHashIt, OValueIt> other)
			: hash_it(other.hash_it), hash_end(other.hash_end), value_it(other.value_it)
		{
		}
		O & operator*() const
		{
			return reinterpret_cast<O &>(*value_it);
		}
		O * operator->() const
		{
			return reinterpret_cast<O *>(&*value_it);
		}
		templated_iterator & operator++()
		{
			return *this = templated_iterator(hash_it + ptrdiff_t(1), hash_end, value_it + ptrdiff_t(1));
		}
		templated_iterator operator++(int)
		{
			templated_iterator copy(*this);
			++*this;
			return copy;
		}
		bool operator==(const templated_iterator & other) const
		{
			return hash_it == other.hash_it;
		}
		bool operator!=(const templated_iterator & other) const
		{
			return !(*this == other);
		}
		template<typename OO, typename OHashIt, typename OValueIt>
		bool operator==(const templated_iterator<OO, OHashIt, OValueIt> & other) const
		{
			return hash_it == other.hash_it;
		}
		template<typename OO, typename OHashIt, typename OValueIt>
		bool operator!=(const templated_iterator<OO, OHashIt, OValueIt> & other) const
		{
			return !(*this == other);
		}

	private:
		friend class sherwood_map;
		HashIt hash_it;
		HashIt hash_end;
		ValueIt value_it;
	};
public:
	typedef templated_iterator<value_type, hash_pointer, value_pointer> iterator;
	typedef templated_iterator<const value_type, const_hash_pointer, const_value_pointer> const_iterator;
private:
	struct HashStorageType : hash_alloc
	{
		HashStorageType(size_t size = 0, const Allocator & alloc = Allocator())
			: hash_alloc(alloc), hash_begin(), hash_end()
		{
			if (size)
			{
				hash_begin = hash_allocator_traits::allocate(*this, size);
				hash_end = hash_begin + ptrdiff_t(size);
				std::fill(hash_begin, hash_end, detail::empty_hash);
			}
		}
		HashStorageType(const HashStorageType & other, const Allocator & alloc)
			: HashStorageType(other.capacity(), alloc)
		{
		}
		HashStorageType(const HashStorageType & other)
			: HashStorageType(other, static_cast<const hash_alloc &>(other))
		{
		}
		HashStorageType(HashStorageType && other, const Allocator & alloc)
			: hash_alloc(alloc)
			, hash_begin(other.hash_begin), hash_end(other.hash_end)
		{
			other.hash_begin = other.hash_end = hash_pointer();
		}
		HashStorageType(HashStorageType && other) noexcept(std::is_nothrow_move_constructible<hash_alloc>::value)
			: hash_alloc(static_cast<hash_alloc &&>(other))
			, hash_begin(other.hash_begin), hash_end(other.hash_end)
		{
			other.hash_begin = other.hash_end = hash_pointer();
		}
		HashStorageType & operator=(const HashStorageType & other)
		{
			HashStorageType(other).swap(*this);
			return *this;
		}
		HashStorageType & operator=(HashStorageType && other) noexcept(std::is_nothrow_move_assignable<hash_pointer>::value && std::is_nothrow_move_assignable<hash_alloc>::value)
		{
			swap(other);
			return *this;
		}
		~HashStorageType()
		{
			if (hash_begin) hash_allocator_traits::deallocate(*this, hash_begin, capacity());
		}
		void swap(HashStorageType & other) noexcept(std::is_nothrow_move_assignable<hash_pointer>::value && std::is_nothrow_move_assignable<hash_alloc>::value)
		{
			using std::swap;
			swap(static_cast<hash_alloc &>(*this), static_cast<hash_alloc &>(other));
			swap(hash_begin, other.hash_begin);
			swap(hash_end, other.hash_end);
		}

		size_t capacity() const
		{
			return hash_end - hash_begin;
		}

		hash_pointer hash_begin;
		hash_pointer hash_end;
	};

	template<typename... Args>
	static void init_empty(value_alloc & alloc, hash_pointer hash_it, value_pointer value_it, size_t hash, Args &&... args)
	{
		value_allocator_traits::construct(alloc, std::addressof(*value_it), std::forward<Args>(args)...);
		*hash_it = hash;
	}

	struct StorageType : HashStorageType, value_alloc, KeyOrValueHasher, KeyOrValueEquality
	{
		template<typename HashIt, typename ValueIt>
		struct storage_iterator : std::iterator<std::random_access_iterator_tag, void, ptrdiff_t, void, void>
		{
			storage_iterator(HashIt hash_it, ValueIt value_it)
				: hash_it(hash_it), value_it(value_it)
			{
			}
			storage_iterator(typename sherwood_map::iterator it)
				: hash_it(it.hash_it), value_it(it.value_it)
			{
			}
			storage_iterator & operator++()
			{
				++hash_it;
				++value_it;
				return *this;
			}
			storage_iterator operator++(int)
			{
				storage_iterator before = *this;
				++*this;
				return before;
			}
			storage_iterator & operator--()
			{
				--hash_it;
				--value_it;
				return *this;
			}
			storage_iterator operator--(int)
			{
				storage_iterator before = *this;
				--*this;
				return before;
			}
			storage_iterator & operator+=(ptrdiff_t distance)
			{
				hash_it += distance;
				value_it += distance;
				return *this;
			}
			storage_iterator & operator-=(ptrdiff_t distance)
			{
				hash_it -= distance;
				value_it -= distance;
				return *this;
			}
			ptrdiff_t operator-(const storage_iterator & other) const
			{
				return hash_it - other.hash_it;
			}
			storage_iterator operator-(ptrdiff_t distance) const
			{
				return { hash_it - distance, value_it - distance };
			}
			storage_iterator operator+(ptrdiff_t distance) const
			{
				return { hash_it + distance, value_it + distance };
			}
			bool operator==(const storage_iterator & other) const
			{
				return hash_it == other.hash_it;
			}
			bool operator!=(const storage_iterator & other) const
			{
				return !(*this == other);
			}
			bool operator<(const storage_iterator & other) const
			{
				return hash_it < other.hash_it;
			}
			bool operator<=(const storage_iterator & other) const
			{
				return !(other < *this);
			}
			bool operator>(const storage_iterator & other) const
			{
				return other < *this;
			}
			bool operator>=(const storage_iterator & other) const
			{
				return !(*this < other);
			}
			template<typename OHashIt, typename OValueIt>
			operator storage_iterator<OHashIt, OValueIt>()
			{
				return { hash_it, value_it };
			}
			template<typename OHashIt, typename OValueIt>
			operator storage_iterator<OHashIt, OValueIt>() const
			{
				return { hash_it, value_it };
			}

			HashIt hash_it;
			ValueIt value_it;
		};

		typedef storage_iterator<hash_pointer, value_pointer> iterator;
		typedef storage_iterator<const_hash_pointer, const_value_pointer> const_iterator;

		explicit StorageType(size_t size = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & alloc = Allocator())
			: HashStorageType(size, alloc)
			, value_alloc(alloc)
			, KeyOrValueHasher(hash)
			, KeyOrValueEquality(equality)
			, value_begin()
		{
			if (size)
			{
				value_begin = value_allocator_traits::allocate(*this, size);
			}
		}
		StorageType(const StorageType & other, const Allocator & alloc)
			: StorageType(other.capacity(), static_cast<const hasher &>(other), static_cast<const key_equal &>(other), alloc)
		{
		}
		StorageType(const StorageType & other)
			: StorageType(other, static_cast<const value_alloc &>(other))
		{
		}
		StorageType(StorageType && other, const Allocator & alloc)
			: HashStorageType(std::move(other), alloc)
			, value_alloc(alloc)
			, KeyOrValueHasher(static_cast<KeyOrValueHasher &&>(other))
			, KeyOrValueEquality(static_cast<KeyOrValueEquality &&>(other))
			, value_begin(other.value_begin)
		{
			other.value_begin = value_pointer();
		}
		StorageType(StorageType && other) noexcept(std::is_nothrow_move_constructible<HashStorageType>::value && std::is_nothrow_move_constructible<value_alloc>::value && std::is_nothrow_move_constructible<hasher>::value && std::is_nothrow_move_constructible<key_equal>::value && std::is_nothrow_move_assignable<value_pointer>::value)
			: HashStorageType(std::move(other))
			, value_alloc(static_cast<value_alloc &&>(other))
			, KeyOrValueHasher(static_cast<KeyOrValueHasher &&>(other))
			, KeyOrValueEquality(static_cast<KeyOrValueEquality &&>(other))
			, value_begin(other.value_begin)
		{
			other.value_begin = value_pointer();
		}
		StorageType & operator=(const StorageType & other)
		{
			StorageType(other).swap(*this);
			return *this;
		}
		StorageType & operator=(StorageType && other) noexcept(std::is_nothrow_move_constructible<HashStorageType>::value && std::is_nothrow_move_assignable<hasher>::value && std::is_nothrow_move_assignable<key_equal>::value && std::is_nothrow_move_assignable<value_alloc>::value && std::is_nothrow_move_assignable<value_pointer>::value)
		{
			swap(other);
			return *this;
		}
		void swap(StorageType & other) noexcept(std::is_nothrow_move_constructible<HashStorageType>::value && std::is_nothrow_move_assignable<hasher>::value && std::is_nothrow_move_assignable<key_equal>::value && std::is_nothrow_move_assignable<value_alloc>::value && std::is_nothrow_move_assignable<value_pointer>::value)
		{
			using std::swap;
			swap(static_cast<hasher &>(static_cast<KeyOrValueHasher &>(*this)), static_cast<hasher &>(static_cast<KeyOrValueHasher &>(other)));
			swap(static_cast<key_equal &>(static_cast<KeyOrValueEquality &>(*this)), static_cast<key_equal &>(static_cast<KeyOrValueEquality &>(other)));
			swap(static_cast<value_alloc &>(*this), static_cast<value_alloc &>(other));
			static_cast<HashStorageType &>(*this).swap(other);
			swap(value_begin, other.value_begin);
		}
		~StorageType()
		{
			if (!value_begin)
				return;
			clear();
			value_allocator_traits::deallocate(*this, value_begin, this->capacity());
		}
		void iterator_destroy(iterator it)
		{
			value_allocator_traits::destroy(*this, std::addressof(*it.value_it));
			*it.hash_it = detail::empty_hash;
		}

		void clear()
		{
			for (auto it = begin(), end = this->end(); it != end; ++it)
			{
				if (*it.hash_it != detail::empty_hash) iterator_destroy(it);
			}
		}

		hasher hash_function() const
		{
			return static_cast<const KeyOrValueHasher &>(*this);
		}
		key_equal key_eq() const
		{
			return static_cast<const KeyOrValueEquality &>(*this);
		}
		value_alloc get_allocator() const
		{
			return static_cast<const value_alloc &>(*this);
		}

		typedef detail::WrappingIterator<typename StorageType::iterator> WrapAroundIt;

		static bool iterator_in_range(iterator check, iterator first, iterator last)
		{
			if (last < first) return check < last || check >= first;
			else return check >= first && check < last;
		}
		void erase_advance(WrapAroundIt & it, iterator target)
		{
			for (; it.it != target; ++it)
			{
				if (*it.it.hash_it != detail::empty_hash)
				{
					iterator_destroy(it.it);
				}
			}
		}
		static void iter_swap(iterator lhs, iterator rhs)
		{
			std::iter_swap(lhs.value_it, rhs.value_it);
			std::iter_swap(lhs.hash_it, rhs.hash_it);
		}
		iterator erase(iterator first, iterator last)
		{
			if (first == last) return first;
			if (last == end())
			{
				last = begin();
				if (first == last)
				{
					clear();
					return end();
				}
			}
			size_t capacity_copy = this->capacity();
			WrapAroundIt first_wrap{ first, begin(), end() };
			WrapAroundIt last_wrap{ last, begin(), end() };
			for (;; ++first_wrap)
			{
				if (*last_wrap.it.hash_it == detail::empty_hash)
				{
					erase_advance(first_wrap, last_wrap.it);
					return first;
				}
				iterator last_initial = initial_bucket(*last_wrap.it.hash_it, capacity_copy);
				WrapAroundIt last_next = std::next(last_wrap);
				if (iterator_in_range(last_initial, std::next(first_wrap).it, last_next.it))
				{
					erase_advance(first_wrap, last_initial);
					if (first_wrap == last_wrap) return first;
				}
				iter_swap(first_wrap.it, last_wrap.it);
				last_wrap = last_next;
			}
		}
		iterator initial_bucket(size_type hash, size_t capacity)
		{
			return begin() + hash % capacity;
		}
		const_iterator initial_bucket(size_type hash, size_t capacity) const
		{
			return begin() + hash % capacity;
		}
		size_t distance_to_initial_bucket(hash_pointer it, size_t hash, size_t capacity) const
		{
			const_hash_pointer initial = initial_bucket(hash, capacity).hash_it;
			if (const_hash_pointer(it) < initial) return (const_hash_pointer(this->hash_end) - initial) + (it - this->hash_begin);
			else return const_hash_pointer(it) - initial;
		}

		enum FindResult
		{
			FoundEmpty,
			FoundEqual,
			FoundNotEqual0
		};

		template<typename First>
		std::pair<iterator, FindResult> find_hash(size_type hash, const First & first)
		{
			size_t capacity_copy = this->capacity();
			if (!capacity_copy)
			{
				iterator end{ this->hash_end, value_end() };
				return { end, FoundNotEqual0 };
			}
			WrapAroundIt it{initial_bucket(hash, capacity_copy), begin(), end()};
			size_t current_hash = *it.it.hash_it;
			if (current_hash == detail::empty_hash)
				return { it.it, FoundEmpty };
			if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it.value_it->first, first))
				return { it.it, FoundEqual };
			for (size_t distance = 1;; ++distance)
			{
				++it;
				current_hash = *it.it.hash_it;
				if (current_hash == detail::empty_hash)
					return { it.it, FoundEmpty };
				if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it.value_it->first, first))
					return { it.it, FoundEqual };
				if (distance_to_initial_bucket(it.it.hash_it, current_hash, capacity_copy) < distance)
					return { it.it, FindResult(FoundNotEqual0 + distance) };
			}
		}
		template<typename First>
		std::pair<const_iterator, FindResult> find_hash(size_type hash, const First & first) const
		{
			return const_cast<sherwood_map &>(*this).find_hash(hash, first);
		}

		value_pointer value_begin;
		value_pointer value_end() const
		{
			return {};
		}

		iterator begin()				{ return { this->hash_begin,	value_begin }; }
		iterator end()					{ return { this->hash_end,		value_end() }; }
		const_iterator begin()	const	{ return { this->hash_begin,	value_begin }; }
		const_iterator end()	const	{ return { this->hash_end,		value_end() }; }
	};

public:
	sherwood_map() = default;
	sherwood_map(const sherwood_map & other)
		: sherwood_map(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
	{
	}
	sherwood_map(const sherwood_map & other, const Allocator & alloc)
		: entries(other.entries, alloc), _max_load_factor(other._max_load_factor)
	{
		insert(other.begin(), other.end());
	}
	sherwood_map(sherwood_map && other) noexcept(std::is_nothrow_move_constructible<StorageType>::value)
		: entries(std::move(other.entries))
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	sherwood_map(sherwood_map && other, const Allocator & alloc)
		: entries(std::move(other.entries), alloc)
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	sherwood_map & operator=(const sherwood_map & other)
	{
		sherwood_map(other).swap(*this);
		return *this;
	}
	sherwood_map & operator=(sherwood_map && other) = default;
	sherwood_map & operator=(std::initializer_list<value_type> il)
	{
		sherwood_map(il).swap(*this);
		return *this;
	}
	explicit sherwood_map(size_t bucket_count, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, allocator)
	{
	}
	explicit sherwood_map(const Allocator & allocator)
		: sherwood_map(0, allocator)
	{
	}
	explicit sherwood_map(size_t bucket_count, const Allocator & allocator)
		: sherwood_map(bucket_count, hasher(), allocator)
	{
	}
	sherwood_map(size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: sherwood_map(bucket_count, hash, key_equal(), allocator)
	{
	}
	template<typename It>
	sherwood_map(It begin, It end, size_t bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, allocator)
	{
		insert(begin, end);
	}
	template<typename It>
	sherwood_map(It begin, It end, size_t bucket_count, const Allocator & allocator)
		: sherwood_map(begin, end, bucket_count, hasher(), allocator)
	{
	}
	template<typename It>
	sherwood_map(It begin, It end, size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: sherwood_map(begin, end, bucket_count, hash, key_equal(), allocator)
	{
	}
	sherwood_map(std::initializer_list<value_type> il, size_type bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: sherwood_map(il.begin(), il.end(), bucket_count, hash, equality, allocator)
	{
	}
	sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const hasher & hash, const Allocator & allocator)
		: sherwood_map(il, bucket_count, hash, key_equal(), allocator)
	{
	}
	sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const Allocator & allocator)
		: sherwood_map(il, bucket_count, hasher(), allocator)
	{
	}

	iterator begin()				{ return { entries.hash_begin,	entries.hash_end, entries.value_begin	}; }
	iterator end()					{ return { entries.hash_end,	entries.hash_end, entries.value_end()	}; }
	const_iterator begin()	const	{ return { entries.hash_begin,	entries.hash_end, entries.value_begin	}; }
	const_iterator end()	const	{ return { entries.hash_end,	entries.hash_end, entries.value_end()	}; }
	const_iterator cbegin()	const	{ return { entries.hash_begin,	entries.hash_end, entries.value_begin	}; }
	const_iterator cend()	const	{ return { entries.hash_end,	entries.hash_end, entries.value_end()	}; }

	bool empty() const
	{
		return _size == 0;
	}
	size_type size() const
	{
		return _size;
	}
	void clear()
	{
		entries.clear();
		_size = 0;
	}
	std::pair<iterator, bool> insert(const value_type & value)
	{
		return emplace(value);
	}
	std::pair<iterator, bool> insert(value_type && value)
	{
		return emplace(std::move(value));
	}
	iterator insert(const_iterator hint, const value_type & value)
	{
		return emplace_hint(hint, value);
	}
	iterator insert(const_iterator hint, value_type && value)
	{
		return emplace_hint(hint, std::move(value));
	}
	template<typename It>
	void insert(It begin, It end)
	{
		for (; begin != end; ++begin)
		{
			emplace(*begin);
		}
	}
	void insert(std::initializer_list<value_type> il)
	{
		insert(il.begin(), il.end());
	}
	template<typename First, typename... Args>
	std::pair<iterator, bool> emplace(First && first, Args &&... args)
	{
		size_type hash = static_cast<KeyOrValueHasher &>(entries)(first);
		auto found = entries.find_hash(hash, first);
		if (found.second == StorageType::FoundEqual)
		{
			return { { found.first.hash_it, entries.hash_end, found.first.value_it }, false };
		}
		if (size() + 1 > _max_load_factor * entries.capacity())
		{
			grow();
			found = entries.find_hash(hash, first);
		}
		switch(found.second)
		{
		case StorageType::FoundEqual:
			throw detail::invalid_code_in_emplace();
		case StorageType::FoundEmpty:
			init_empty(entries, found.first.hash_it, found.first.value_it, hash, std::forward<First>(first), std::forward<Args>(args)...);
			++_size;
			return { { found.first.hash_it, entries.hash_end, found.first.value_it }, true };
		default:
			value_type new_value(std::forward<First>(first), std::forward<Args>(args)...);
			size_t & current_hash = *found.first.hash_it;
			value_type & current_value = *found.first.value_it;
			typename StorageType::WrapAroundIt next{found.first, entries.begin(), entries.end()};
			++next;
			if (*next.it.hash_it != detail::empty_hash)
			{
				size_t capacity = entries.capacity();
				size_t distance = found.second - StorageType::FoundNotEqual0;//entries.distance_to_initial_bucket(found.first.hash_it, current_hash, capacity) + 1;
				do
				{
					size_t next_distance = entries.distance_to_initial_bucket(next.it.hash_it, *next.it.hash_it, capacity);
					if (next_distance < distance)
					{
						distance = next_distance;
						StorageType::iter_swap(found.first, next.it);
					}
					++distance;
					++next;
				}
				while (*next.it.hash_it != detail::empty_hash);
			}
			init_empty(entries, next.it.hash_it, next.it.value_it, current_hash, std::move(current_value));
			current_value = std::move(new_value);
			current_hash = hash;
			++_size;
			return { { found.first.hash_it, entries.hash_end, found.first.value_it }, true };
		}
	}
	std::pair<iterator, bool> emplace()
	{
		return emplace(value_type());
	}
	template<typename... Args>
	iterator emplace_hint(const_iterator, Args &&... args)
	{
		return emplace(std::forward<Args>(args)...).first;
	}
	iterator erase(const_iterator pos)
	{
		--_size;
		typename StorageType::iterator non_const_pos(iterator_const_cast(pos));
		auto erased = entries.erase(non_const_pos, std::next(non_const_pos));
		return { erased.hash_it, entries.hash_end, erased.value_it };
	}
	iterator erase(const_iterator first, const_iterator last)
	{
		_size -= std::distance(first, last);
		auto erased = entries.erase(iterator_const_cast(first), iterator_const_cast(last));
		return { erased.hash_it, entries.hash_end, erased.value_it };
	}
	size_type erase(const key_type & key)
	{
		auto found = find(key);
		if (found == end()) return 0;
		else
		{
			erase(found);
			return 1;
		}
	}

	mapped_type & operator[](const key_type & key)
	{
		return emplace(key, detail::lazily_defauly_construct<mapped_type>()).first->second;
	}
	mapped_type & operator[](key_type && key)
	{
		return emplace(std::move(key), detail::lazily_defauly_construct<mapped_type>()).first->second;
	}
	template<typename T>
	iterator find(const T & key)
	{
		size_t hash = static_cast<KeyOrValueHasher &>(entries)(key);
		auto found = entries.find_hash(hash, key);
		if (found.second == StorageType::FoundEqual) return { found.first.hash_it, entries.hash_end, found.first.value_it };
		else return end();
	}
	template<typename T>
	const_iterator find(const T & key) const
	{
		return const_cast<sherwood_map &>(*this).find(key);
	}
	template<typename T>
	mapped_type & at(const T & key)
	{
		auto found = find(key);
		if (found == end()) throw detail::at_out_of_range();
		else return found->second;
	}
	template<typename T>
	const mapped_type & at(const T & key) const
	{
		return const_cast<sherwood_map &>(*this).at(key);
	}
	template<typename T>
	size_type count(const T & key) const
	{
		return find(key) == end() ? 0 : 1;
	}
	template<typename T>
	std::pair<iterator, iterator> equal_range(const T & key)
	{
		auto found = find(key);
		if (found == end()) return { found, found };
		else return { found, std::next(found) };
	}
	template<typename T>
	std::pair<const_iterator, const_iterator> equal_range(const T & key) const
	{
		return const_cast<sherwood_map &>(*this).equal_range(key);
	}
	void swap(sherwood_map & other)
	{
		using std::swap;
		entries.swap(other.entries);
		swap(_size, other._size);
		swap(_max_load_factor, other._max_load_factor);
	}
	float load_factor() const
	{
		size_type capacity_copy = entries.capacity();
		if (capacity_copy) return static_cast<float>(size()) / capacity_copy;
		else return 0.0f;
	}
	float max_load_factor() const
	{
		return _max_load_factor;
	}
	void max_load_factor(float value)
	{
		if (value >= 0.01f && value <= 1.0f)
		{
			_max_load_factor = value;
		}
		else
		{
			throw detail::invalid_max_load_factor();
		}
	}
	void reserve(size_type count)
	{
		size_t new_size = detail::required_capacity(count, max_load_factor());
		if (new_size > entries.capacity()) reallocate(detail::next_prime(new_size));
	}
	void rehash(size_type count)
	{
		if (count < size() / max_load_factor()) count = detail::next_prime(size_type(std::ceil(size() / max_load_factor())));
		reallocate(count);
	}
	size_type bucket(const key_type & key) const
	{
		return entries.initial_bucket(static_cast<const KeyOrValueHasher &>(entries)(key), entries.capacity()) - entries.begin();
	}
	size_type bucket_count() const
	{
		return entries.capacity();
	}
	size_type max_bucket_count() const
	{
		return (value_allocator_traits::max_size(entries) - sizeof(*this)) / (sizeof(size_t) + sizeof(value_type));
	}
	bool operator==(const sherwood_map & other) const
	{
		if (size() != other.size()) return false;
		return std::all_of(begin(), end(), [&other](const value_type & value)
		{
			auto found = other.find(value.first);
			return found != other.end() && *found == value;
		});
	}
	bool operator!=(const sherwood_map & other) const
	{
		return !(*this == other);
	}

	key_equal key_eq() const
	{
		return entries;
	}
	hasher hash_function() const
	{
		return entries;
	}
	allocator_type get_allocator() const
	{
		return entries.get_allocator();
	}

private:
	void grow()
	{
		reallocate(detail::next_prime(std::max(detail::required_capacity(_size + 1, _max_load_factor), entries.capacity() * 2)));
	}
	void reallocate(size_type size) __attribute__((noinline))
	{
		StorageType replacement(size, entries.hash_function(), entries.key_eq(), entries.get_allocator());
		entries.swap(replacement);
		_size = 0;
		auto value_it = replacement.value_begin;
		for (auto it = replacement.hash_begin, end = replacement.hash_end; it != end; ++it, ++value_it)
		{
			if (*it != detail::empty_hash)
				emplace(std::move(*value_it));
		}
	}

	iterator iterator_const_cast(const_iterator it)
	{
		union caster
		{
			caster(const_iterator it)
				: it(it)
			{
			}
			~caster()
			{
				it.~const_iterator();
			}
			iterator non_const;
			const_iterator it;
		};
		static_assert(sizeof(iterator) == sizeof(const_iterator), "doing reinterpret_cast here");
		return caster(it).non_const;
	}

	StorageType entries;
	size_t _size = 0;
	static constexpr const float default_load_factor = 0.85f;
	float _max_load_factor = default_load_factor;
};
