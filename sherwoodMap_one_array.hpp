#pragma once

#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cstddef>

namespace detail
{
#ifdef PROFILE_SHERWOOD_MAP
extern size_t profile_cost;
#endif
void throw_sherwood_map_out_of_range();
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
template<typename It, typename Do, typename Undo>
void exception_safe_for_each(It begin, It end, Do && do_func, Undo && undo_func)
{
	for (It it = begin; it != end; ++it)
	{
		try
		{
			do_func(*it);
		}
		catch(...)
		{
			while (it != begin)
			{
				undo_func(*--it);
			}
			throw;
		}
	}
}
}

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equality = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<const Key, Value> > >
class sherwood_map
{
public:
	typedef Key key_type;
	typedef Value mapped_type;
	typedef std::pair<const Key, Value> value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef Hash hasher;
	typedef Equality key_equal;
	typedef Allocator allocator_type;
	typedef value_type & reference;
	typedef const value_type & const_reference;
private:
	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const Key, Value> > pretend_alloc;
public:
	typedef typename std::allocator_traits<pretend_alloc>::pointer pointer;
	typedef typename std::allocator_traits<pretend_alloc>::const_pointer const_pointer;

private:
	struct Entry;

	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<Entry> actual_alloc;
	typedef std::allocator_traits<actual_alloc> allocator_traits;

	template<typename T, typename It>
	struct WrappingIterator
	{
		T & operator*() const
		{
			return *it;
		}
		T * operator->() const
		{
			return &*it;
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

		operator It() const
		{
			return it;
		}

		It it;
		It begin;
		It end;
	};

	static constexpr size_t special_value = 0;
	static size_t adjust_for_special_value(size_t hash)
	{
		return std::max(size_t(1), hash);
	}

	typedef detail::functor_storage<size_t, hasher> hasher_storage;
	typedef detail::functor_storage<bool, key_equal> equality_storage;
	struct KeyOrValueHasher : hasher_storage
	{
		KeyOrValueHasher(const hasher & hash)
			: hasher_storage(hash)
		{
		}
		size_t operator()(const key_type & key)
		{
			return adjust_for_special_value(static_cast<hasher_storage &>(*this)(key));
		}
		size_t operator()(const key_type & key) const
		{
			return adjust_for_special_value(static_cast<const hasher_storage &>(*this)(key));
		}
		size_t operator()(const value_type & value)
		{
			return adjust_for_special_value(static_cast<hasher_storage &>(*this)(value.first));
		}
		size_t operator()(const value_type & value) const
		{
			return adjust_for_special_value(static_cast<const hasher_storage &>(*this)(value.first));
		}
		template<typename F, typename S>
		size_t operator()(const std::pair<F, S> & value)
		{
			return adjust_for_special_value(static_cast<hasher_storage &>(*this)(value.first));
		}
		template<typename F, typename S>
		size_t operator()(const std::pair<F, S> & value) const
		{
			return adjust_for_special_value(static_cast<const hasher_storage &>(*this)(value.first));
		}
	};
	struct KeyOrValueEquality : equality_storage
	{
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

	struct StorageType : actual_alloc, KeyOrValueHasher, KeyOrValueEquality
	{
		typedef typename allocator_traits::pointer iterator;
		typedef typename allocator_traits::const_pointer const_iterator;

		explicit StorageType(size_t size = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const actual_alloc & alloc = actual_alloc())
			: actual_alloc(std::move(alloc))
			, KeyOrValueHasher(hash)
			, KeyOrValueEquality(equality)
			, _begin(), _end()
		{
			if (size)
			{
				_begin = allocator_traits::allocate(*this, size);
				_end = _begin + static_cast<ptrdiff_t>(size);
			}
			try
			{
				detail::exception_safe_for_each(begin(), end(), [this](Entry & entry)
				{
					allocator_traits::construct(*this, &entry);
				},
				[this](Entry & entry)
				{
					allocator_traits::destroy(*this, &entry);
				});
			}
			catch(...)
			{
				allocator_traits::deallocate(*this, _begin, size);
				throw;
			}
		}
		StorageType(const StorageType & other, const actual_alloc & alloc)
			: StorageType(other.capacity(), static_cast<const hasher &>(other), static_cast<const key_equal &>(other), alloc)
		{
		}
		StorageType(const StorageType & other)
			: StorageType(other, static_cast<const actual_alloc &>(other))
		{
		}
		StorageType(StorageType && other, const actual_alloc & alloc)
			: actual_alloc(alloc)
			, KeyOrValueHasher(static_cast<KeyOrValueHasher &&>(other))
			, KeyOrValueEquality(static_cast<KeyOrValueEquality &&>(other))
			, _begin(other._begin), _end(other._end)
		{
			other._begin = other._end = iterator();
		}
		StorageType(StorageType && other) noexcept(std::is_nothrow_move_constructible<actual_alloc>::value && std::is_nothrow_move_constructible<hasher>::value && std::is_nothrow_move_constructible<key_equal>::value)
			: actual_alloc(static_cast<actual_alloc &&>(other))
			, KeyOrValueHasher(static_cast<KeyOrValueHasher &&>(other))
			, KeyOrValueEquality(static_cast<KeyOrValueEquality &&>(other))
			, _begin(other._begin), _end(other._end)
		{
			other._begin = other._end = iterator();
		}
		StorageType & operator=(const StorageType & other)
		{
			StorageType(other).swap(*this);
			return *this;
		}
		StorageType & operator=(StorageType && other) noexcept(noexcept(this->swap(other)))
		{
			swap(other);
			return *this;
		}
		void swap(StorageType & other) noexcept(std::is_nothrow_move_assignable<hasher>::value && std::is_nothrow_move_assignable<key_equal>::value && std::is_nothrow_move_assignable<actual_alloc>::value && std::is_nothrow_move_assignable<typename allocator_traits::pointer>::value)
		{
			using std::swap;
			swap(static_cast<hasher &>(static_cast<KeyOrValueHasher &>(*this)), static_cast<hasher &>(static_cast<KeyOrValueHasher &>(other)));
			swap(static_cast<key_equal &>(static_cast<KeyOrValueEquality &>(*this)), static_cast<key_equal &>(static_cast<KeyOrValueEquality &>(other)));
			swap(static_cast<actual_alloc &>(*this), static_cast<actual_alloc &>(other));
			swap(_begin, other._begin);
			swap(_end, other._end);
		}
		~StorageType()
		{
			if (!_begin)
				return;
			for (Entry & entry : *this)
				allocator_traits::destroy(*this, &entry);
			allocator_traits::deallocate(*this, _begin, capacity());
		}

		hasher hash_function() const
		{
			return static_cast<const KeyOrValueHasher &>(*this);
		}
		key_equal key_eq() const
		{
			return static_cast<const KeyOrValueEquality &>(*this);
		}
		actual_alloc get_allocator() const
		{
			return static_cast<const actual_alloc &>(*this);
		}

		size_t capacity() const
		{
			return _end - _begin;
		}

		iterator begin() { return _begin; }
		iterator end() { return _end; }
		const_iterator begin() const { return _begin; }
		const_iterator end() const { return _end; }

		iterator erase(iterator first, iterator last)
		{
			std::fill(first, last, Entry());
			auto stop = find_stop_bucket(last);
			if (stop < last)
			{
				std::rotate(first, last, _end);
				difference_type num_to_move = last - first;
				auto middle = std::min(stop, _begin + num_to_move);
				std::swap_ranges(_begin, middle, _end - num_to_move);
				std::rotate(_begin, middle, stop);
			}
			else std::rotate(first, last, stop);
			return first;
		}
		iterator initial_bucket(size_type hash, size_t capacity)
		{
			return _begin + ptrdiff_t(hash % capacity);
		}
		const_iterator initial_bucket(size_type hash, size_t capacity) const
		{
			return _begin + ptrdiff_t(hash % capacity);
		}
		size_t distance_to_initial_bucket(iterator it, size_t hash, size_t capacity) const
		{
			const_iterator initial = initial_bucket(hash, capacity);
			if (const_iterator(it) < initial) return (const_iterator(_end) - initial) + (it - _begin);
			else return const_iterator(it) - initial;
		}
		enum FindResult
		{
			FoundEmpty,
			FoundEqual,
			FoundNotEqual
		};

		typedef WrappingIterator<Entry, typename StorageType::iterator> WrapAroundIt;

		template<typename First>
		std::pair<iterator, FindResult> find_hash(size_type hash, const First & first)
		{
			size_t capacity_copy = capacity();
			if (!capacity_copy) return { end(), FoundNotEqual };
			size_t distance = 0;
			for (WrapAroundIt it{initial_bucket(hash, capacity_copy), begin(), end()};; ++it, ++distance)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				if (it->is_empty())
					return { it, FoundEmpty };
				size_t current_hash = it->get_hash();
				if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it->get_entry().first, first))
					return { it, FoundEqual };
				if (distance_to_initial_bucket(it, current_hash, capacity_copy) < distance)
					return { it, FoundNotEqual };
			}
		}
		template<typename First>
		std::pair<const_iterator, FindResult> find_hash(size_type hash, const First & first) const
		{
			return const_cast<sherwood_map &>(*this).find_hash(hash, first);
		}

	private:
		typename allocator_traits::pointer _begin;
		typename allocator_traits::pointer _end;

		iterator find_stop_bucket(iterator start)
		{
			size_t capacity_copy = capacity();
			if (start == end())
				start = begin();
			for (WrapAroundIt it{start, begin(), end()};; ++it)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				if (it->is_empty() || distance_to_initial_bucket(it, it->get_hash(), capacity_copy) == 0)
					return it;
			}
		}
	};

	template<typename O, typename It>
	struct templated_iterator : std::iterator<std::forward_iterator_tag, O, ptrdiff_t>
	{
		templated_iterator()
			: it(), end()
		{
		}
		templated_iterator(It it, It end)
			: it(it), end(end)
		{
			for (; this->it != this->end && this->it->is_empty();)
			{
				++this->it;
			}
		}
		O & operator*() const
		{
			return it->get_entry();
		}
		O * operator->() const
		{
			return &it->get_entry();
		}
		templated_iterator & operator++()
		{
			return *this = templated_iterator(it + ptrdiff_t(1), end);
		}
		templated_iterator operator++(int)
		{
			templated_iterator copy(*this);
			++*this;
			return copy;
		}
		bool operator==(const templated_iterator & other) const
		{
			return it == other.it;
		}
		bool operator!=(const templated_iterator & other) const
		{
			return !(*this == other);
		}
		template<typename OO, typename OIt>
		bool operator==(const templated_iterator<OO, OIt> & other) const
		{
			return it == other.it;
		}
		template<typename OO, typename OIt>
		bool operator!=(const templated_iterator<OO, OIt> & other) const
		{
			return !(*this == other);
		}

		template<typename OO, typename OIt>
		operator templated_iterator<OO, OIt>() const
		{
			return { it, end };
		}

	private:
		friend class sherwood_map;
		It it;
		It end;
	};

public:
	typedef templated_iterator<value_type, typename StorageType::iterator> iterator;
	typedef templated_iterator<const value_type, typename StorageType::const_iterator> const_iterator;

	sherwood_map() = default;
	sherwood_map(const sherwood_map & other)
		: sherwood_map(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
	{
	}
	sherwood_map(const sherwood_map & other, const Allocator & alloc)
		: entries(other.entries, actual_alloc(alloc)), _max_load_factor(other._max_load_factor)
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
		: entries(std::move(other.entries), actual_alloc(alloc))
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

	explicit sherwood_map(size_t bucket_count, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, actual_alloc(allocator))
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
		: entries(bucket_count, hash, equality, actual_alloc(allocator))
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

	iterator begin()				{ return { entries.begin(), entries.end()	}; }
	iterator end()					{ return { entries.end(),	entries.end()	}; }
	const_iterator begin()	const	{ return { entries.begin(), entries.end()	}; }
	const_iterator end()	const	{ return { entries.end(),	entries.end()	}; }
	const_iterator cbegin()	const	{ return { entries.begin(), entries.end()	}; }
	const_iterator cend()	const	{ return { entries.end(),	entries.end()	}; }

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
		for (Entry & entry : entries)
		{
			entry = Entry();
		}
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
			return { { found.first, entries.end() }, false };
		}
		if (size() + 1 > _max_load_factor * entries.capacity())
		{
			grow();
			found = entries.find_hash(hash, first);
		}
		switch(found.second)
		{
		case StorageType::FoundEqual:
			throw std::logic_error("should be impossible to get here because I already handled the FoundEqual case at the beginning of the function");
		case StorageType::FoundEmpty:
			found.first->init(hash, std::forward<First>(first), std::forward<Args>(args)...);
			++_size;
			return { { found.first, entries.end() }, true };
		case StorageType::FoundNotEqual:
			size_t capacity = entries.capacity();
			Entry & current = *found.first;
			size_t distance = entries.distance_to_initial_bucket(found.first, current.get_hash(), capacity);
			typename StorageType::WrapAroundIt next{found.first, entries.begin(), entries.end()};
			for (++distance, ++next; !next->is_empty(); ++distance, ++next)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				size_t next_distance = entries.distance_to_initial_bucket(next, next->get_hash(), capacity);
				if (next_distance < distance)
				{
					distance = next_distance;
					current.swap_non_empty(*next);
				}
			}
			next->init(current.get_hash(), std::move(current.get_entry()));
			current.reinit(hash, std::forward<First>(first), std::forward<Args>(args)...);
			++_size;
			return { { found.first, entries.end() }, true };
		}
		throw std::runtime_error("Forgot to handle a case in the switch statement above");
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
		auto non_const_pos = iterator_const_cast(pos).it;
		auto next = non_const_pos;
		++next;
		return { entries.erase(non_const_pos, next), entries.end() };
	}
	iterator erase(const_iterator first, const_iterator last)
	{
		_size -= std::distance(first, last);
		auto non_const_first = iterator_const_cast(first).it;
		auto non_const_last = iterator_const_cast(last).it;
		return { entries.erase(non_const_first, non_const_last), entries.end() };
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
		auto found = find(key);
		if (found != end()) return found->second;
		else return emplace(key, mapped_type()).first->second;
	}
	mapped_type & operator[](key_type && key)
	{
		auto found = find(key);
		if (found != end()) return found->second;
		else return emplace(std::move(key), mapped_type()).first->second;
	}
	template<typename T>
	iterator find(const T & key)
	{
		size_t hash = static_cast<KeyOrValueHasher &>(entries)(key);
		auto found = entries.find_hash(hash, key);
		if (found.second == StorageType::FoundEqual) return { found.first, entries.end() };
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
		if (found == end()) detail::throw_sherwood_map_out_of_range();
		return found->second;
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
			throw std::runtime_error("invalid value for max_load_factor(). sherwood_map only supports load factors in the range [0.01 .. 1]");
		}
	}
	void reserve(size_type count)
	{
		size_t new_size = required_capacity(count, max_load_factor());
		if (new_size > entries.capacity()) reallocate(new_size);
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
		return (allocator_traits::max_size(entries) - sizeof(*this)) / sizeof(Entry);
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
		return static_cast<const actual_alloc &>(entries);
	}

private:
	void grow()
	{
		reallocate(detail::next_prime(std::max(required_capacity(_size + 1, _max_load_factor), entries.capacity() * 2)));
	}
	void reallocate(size_type size)
	{
		StorageType replacement(size, entries.hash_function(), entries.key_eq(), entries.get_allocator());
		entries.swap(replacement);
		_size = 0;
		for (Entry & entry : replacement)
		{
			if (!entry.is_empty())
				emplace(std::move(entry.get_entry()));
		}
	}

	iterator iterator_const_cast(const_iterator it)
	{
		return { entries.begin() + ptrdiff_t(it.it - entries.begin()), entries.end() };
	}

	struct Entry
	{
		Entry()
			: hash(special_value)
		{
		}
		template<typename... Args>
		void init(size_t hash, Args &&... args)
		{
			this->hash = hash;
			new (&entry) value_type(std::forward<Args>(args)...);
		}
		template<typename... Args>
		void reinit(size_t hash, Args &&... args)
		{
			this->hash = hash;
			value_type as_value_type(std::forward<Args>(args)...);
			// cast away the constness to allow move assignment
			entry = reinterpret_cast<std::pair<Key, Value> &&>(as_value_type);
		}
		Entry(const Entry & other)
			: hash(other.hash)
		{
			if (hash != special_value)
				new (&entry) std::pair<Key, Value>(other.entry);
		}
		Entry(Entry && other)
			: hash(other.hash)
		{
			if (hash != special_value)
				new (&entry) std::pair<Key, Value>(std::move(other.entry));
		}
		Entry & operator=(Entry other)
		{
			if (other.hash == special_value)
			{
				if (hash != special_value)
				{
					hash = special_value;
					entry.~pair();
				}
			}
			else if (hash == special_value)
			{
				new (&entry) std::pair<Key, Value>(std::move(other.entry));
				hash = other.hash;
			}
			else
			{
				entry = std::move(other.entry);
				hash = other.hash;
			}
			return *this;
		}
		~Entry()
		{
			if (hash == special_value)
				return;
			entry.~pair();
		}
		bool is_empty() const
		{
			return hash == special_value;
		}
		std::pair<const Key, Value> & get_entry()
		{
			return reinterpret_cast<std::pair<const Key, Value> &>(entry);
		}
		const std::pair<const Key, Value> & get_entry() const
		{
			return reinterpret_cast<const std::pair<const Key, Value> &>(entry);
		}

		size_t get_hash() const
		{
			return hash;
		}

		void swap_non_empty(Entry & other)
		{
			using std::swap;
			swap(hash, other.hash);
			swap(entry, other.entry);
		}

	private:
		size_t hash;
		union
		{
			char unused;
			std::pair<Key, Value> entry;
		};
	};
	static size_type required_capacity(size_type size, float load_factor)
	{
		return detail::next_prime(size_type(std::ceil(size / load_factor)));
	}

	StorageType entries;
	size_t _size = 0;
	static constexpr const float default_load_factor = 0.85f;
	float _max_load_factor = default_load_factor;
};
