#pragma once

#include "sherwood_map_shared.hpp"

#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <stdexcept>

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equality = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<Key, Value> > >
class thin_sherwood_map
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
	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<value_type> pretend_alloc;
public:
	typedef typename std::allocator_traits<pretend_alloc>::pointer pointer;
	typedef typename std::allocator_traits<pretend_alloc>::const_pointer const_pointer;

private:
	struct Entry;

	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<Entry> actual_alloc;
	typedef std::allocator_traits<actual_alloc> allocator_traits;

	typedef detail::KeyOrValueHasher<key_type, value_type, hasher> KeyOrValueHasher;
	typedef detail::KeyOrValueEquality<key_type, value_type, key_equal> KeyOrValueEquality;

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
		StorageType & operator=(StorageType && other) noexcept(std::is_nothrow_move_assignable<hasher>::value && std::is_nothrow_move_assignable<key_equal>::value && std::is_nothrow_move_assignable<actual_alloc>::value && std::is_nothrow_move_assignable<typename allocator_traits::pointer>::value)
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

		static bool iterator_in_range(iterator check, iterator first, iterator last)
		{
			if (last < first) return check < last || check >= first;
			else return check >= first && check < last;
		}
		static void iterator_advance_to_range(iterator & it, iterator first, iterator last)
		{
			if (last >= first) it = std::max(it, first);
			else if (it >= last) it = std::max(it, first);
		}

		typedef detail::WrappingIterator<typename StorageType::iterator> WrapAroundIt;

		static void erase_advance(WrapAroundIt & it, iterator target)
		{
			for (; it.it != target; ++it)
			{
				*it.it = Entry();
			}
		}

		iterator erase(iterator first, iterator last)
		{
			if (first == last) return first;
			if (last == end())
			{
				last = begin();
				if (first == last)
				{
					std::fill(begin(), end(), Entry());
					return end();
				}
			}
			size_t capacity_copy = capacity();
			WrapAroundIt first_wrap{ first, begin(), end() };
			WrapAroundIt last_wrap{ last, begin(), end() };
			for (;; ++first_wrap)
			{
				if (last_wrap.it->hash == detail::empty_hash)
				{
					erase_advance(first_wrap, last_wrap.it);
					return first;
				}
				iterator last_initial = initial_bucket(last_wrap.it->hash, capacity_copy);
				WrapAroundIt last_next = std::next(last_wrap);
				if (iterator_in_range(last_initial, std::next(first_wrap).it, last_next.it))
				{
					erase_advance(first_wrap, last_initial);
					if (first_wrap == last_wrap) return first;
				}
				std::iter_swap(first_wrap.it, last_wrap.it);
				last_wrap = last_next;
			}
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
			const_iterator initial = _begin + ptrdiff_t(hash % capacity);
			if (const_iterator(it) < initial) return (const_iterator(_end) - initial) + (it - _begin);
			else return const_iterator(it) - initial;
		}
		enum FindResult
		{
			FoundEmpty,
			FoundEqual,
			FoundNotEqual
		};

		template<typename First>
		std::pair<iterator, FindResult> find_hash(size_type hash, const First & first)
		{
			size_t capacity_copy = capacity();
			if (!capacity_copy) return { end(), FoundNotEqual };
			WrapAroundIt it{initial_bucket(hash, capacity_copy), begin(), end()};
			size_t current_hash = it.it->hash;
			if (current_hash == detail::empty_hash)
				return { it.it, FoundEmpty };
			if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it->get_entry().first, first))
				return { it.it, FoundEqual };
			for (size_t distance = 1;; ++distance)
			{
				++it;
				current_hash = it.it->hash;
				if (current_hash == detail::empty_hash)
					return { it.it, FoundEmpty };
				if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it->get_entry().first, first))
					return { it.it, FoundEqual };
				if (distance_to_initial_bucket(it.it, current_hash, capacity_copy) < distance)
					return { it.it, FoundNotEqual };
			}
		}
		template<typename First>
		std::pair<const_iterator, FindResult> find_hash(size_type hash, const First & first) const
		{
			return const_cast<thin_sherwood_map &>(*this).find_hash(hash, first);
		}

	private:
		typename allocator_traits::pointer _begin;
		typename allocator_traits::pointer _end;
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
			for (; this->it != this->end && this->it->hash == detail::empty_hash;)
			{
				++this->it;
			}
		}
		template<typename OO, typename OIt>
		templated_iterator(templated_iterator<OO, OIt> other)
			: it(other.it), end(other.end)
		{
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

	private:
		friend class thin_sherwood_map;
		It it;
		It end;
	};

public:
	typedef templated_iterator<value_type, typename StorageType::iterator> iterator;
	typedef templated_iterator<const value_type, typename StorageType::const_iterator> const_iterator;

	thin_sherwood_map() = default;
	thin_sherwood_map(const thin_sherwood_map & other)
		: thin_sherwood_map(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
	{
	}
	thin_sherwood_map(const thin_sherwood_map & other, const Allocator & alloc)
		: entries(other.entries, actual_alloc(alloc)), _max_load_factor(other._max_load_factor)
	{
		insert(other.begin(), other.end());
	}
	thin_sherwood_map(thin_sherwood_map && other) noexcept(std::is_nothrow_move_constructible<StorageType>::value)
		: entries(std::move(other.entries))
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	thin_sherwood_map(thin_sherwood_map && other, const Allocator & alloc)
		: entries(std::move(other.entries), actual_alloc(alloc))
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	thin_sherwood_map & operator=(const thin_sherwood_map & other)
	{
		thin_sherwood_map(other).swap(*this);
		return *this;
	}
	thin_sherwood_map & operator=(thin_sherwood_map && other) = default;
	thin_sherwood_map & operator=(std::initializer_list<value_type> il)
	{
		thin_sherwood_map(il, 0, hash_function(), key_eq(), get_allocator()).swap(*this);
		return *this;
	}
	explicit thin_sherwood_map(size_t bucket_count, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, actual_alloc(allocator))
	{
	}
	explicit thin_sherwood_map(const Allocator & allocator)
		: thin_sherwood_map(0, allocator)
	{
	}
	explicit thin_sherwood_map(size_t bucket_count, const Allocator & allocator)
		: thin_sherwood_map(bucket_count, hasher(), allocator)
	{
	}
	thin_sherwood_map(size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: thin_sherwood_map(bucket_count, hash, key_equal(), allocator)
	{
	}
	template<typename It>
	thin_sherwood_map(It begin, It end, size_t bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, actual_alloc(allocator))
	{
		insert(begin, end);
	}
	template<typename It>
	thin_sherwood_map(It begin, It end, size_t bucket_count, const Allocator & allocator)
		: thin_sherwood_map(begin, end, bucket_count, hasher(), allocator)
	{
	}
	template<typename It>
	thin_sherwood_map(It begin, It end, size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: thin_sherwood_map(begin, end, bucket_count, hash, key_equal(), allocator)
	{
	}
	thin_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: thin_sherwood_map(il.begin(), il.end(), bucket_count, hash, equality, allocator)
	{
	}
	thin_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const hasher & hash, const Allocator & allocator)
		: thin_sherwood_map(il, bucket_count, hash, key_equal(), allocator)
	{
	}
	thin_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const Allocator & allocator)
		: thin_sherwood_map(il, bucket_count, hasher(), allocator)
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
			// create the object to insert early so that if it throws, nothing bad has happened
			// after this I assume noexcept moves
			value_type to_insert(std::forward<First>(first), std::forward<Args>(args)...);
			size_t capacity = entries.capacity();
			Entry & current = *found.first;
			typename StorageType::WrapAroundIt next{found.first, entries.begin(), entries.end()};
			++next;
			if (next.it->hash != detail::empty_hash)
			{
				size_t distance = entries.distance_to_initial_bucket(found.first, current.hash, capacity) + 1;
				do
				{
					size_t next_distance = entries.distance_to_initial_bucket(next.it, next.it->hash, capacity);
					if (next_distance < distance)
					{
						distance = next_distance;
						current.swap_non_empty(*next.it);
					}
					++distance;
					++next;
				}
				while (next.it->hash != detail::empty_hash);
			}
			next.it->init(current.hash, std::move(current.get_entry()));
			current.reinit(hash, std::move(to_insert));
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
		if (found.second == StorageType::FoundEqual) return { found.first, entries.end() };
		else return end();
	}
	template<typename T>
	const_iterator find(const T & key) const
	{
		return const_cast<thin_sherwood_map &>(*this).find(key);
	}
	template<typename T>
	mapped_type & at(const T & key)
	{
		auto found = find(key);
		if (found == end()) throw std::out_of_range("the key that you passed to the at() function did not exist  in this map");
		else return found->second;
	}
	template<typename T>
	const mapped_type & at(const T & key) const
	{
		return const_cast<thin_sherwood_map &>(*this).at(key);
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
		return const_cast<thin_sherwood_map &>(*this).equal_range(key);
	}
	void swap(thin_sherwood_map & other)
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
		return (allocator_traits::max_size(entries) - sizeof(*this)) / sizeof(Entry);
	}
	bool operator==(const thin_sherwood_map & other) const
	{
		if (size() != other.size()) return false;
		return std::all_of(begin(), end(), [&other](const value_type & value)
		{
			auto found = other.find(value.first);
			return found != other.end() && *found == value;
		});
	}
	bool operator!=(const thin_sherwood_map & other) const
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
		reallocate(detail::next_prime(std::max(detail::required_capacity(_size + 1, _max_load_factor), entries.capacity() * 2)));
	}
	void reallocate(size_type size)
	{
		StorageType replacement(size, entries.hash_function(), entries.key_eq(), entries.get_allocator());
		entries.swap(replacement);
		_size = 0;
		for (Entry & entry : replacement)
		{
			if (entry.hash != detail::empty_hash)
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
			: hash(detail::empty_hash)
		{
		}
		template<typename... Args>
		void init(size_t hash, Args &&... args)
		{
			new (&entry) value_type(std::forward<Args>(args)...);
			this->hash = hash;
		}
		void reinit(size_t hash, value_type && value)
		{
			entry = std::move(value);
			this->hash = hash;
		}
		Entry(const Entry & other)
			: Entry()
		{
			if (other.hash != detail::empty_hash)
				init(other.hash, other.entry);
		}
		Entry(Entry && other)
			: Entry()
		{
			if (other.hash != detail::empty_hash)
				init(other.hash, std::move(other.entry));
		}
		Entry & operator=(Entry other)
		{
			if (other.hash == detail::empty_hash)
			{
				if (hash != detail::empty_hash)
				{
					hash = detail::empty_hash;
					entry.~pair();
				}
			}
			else if (hash == detail::empty_hash)
			{
				init(other.hash, std::move(other.entry));
			}
			else
			{
				reinit(other.hash, std::move(other.entry));
			}
			return *this;
		}
		~Entry()
		{
			if (hash == detail::empty_hash)
				return;
			entry.~pair();
		}
		value_type & get_entry()
		{
			return entry;
		}
		const value_type & get_entry() const
		{
			return entry;
		}

		void swap_non_empty(Entry & other)
		{
			using std::swap;
			swap(hash, other.hash);
			swap(entry, other.entry);
		}

		size_t hash;
	private:
		union
		{
			value_type entry;
		};
	};

	StorageType entries;
	size_t _size = 0;
	static constexpr const float default_load_factor = 0.85f;
	float _max_load_factor = default_load_factor;
};
