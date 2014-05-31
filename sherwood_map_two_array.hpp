#pragma once

#include "finished/sherwood_map.hpp"

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equality = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<Key, Value> > >
class fat_sherwood_map
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
	typedef uint32_t distance_type;
	struct HashAndDistance
	{
		size_t hash;
		distance_type bucket_distance;
		distance_type bucket_size;
	};
	typedef typename std::allocator_traits<Allocator>::template rebind_alloc<HashAndDistance> hash_alloc;
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

	struct AdvanceIterator{};
	struct DoNotAdvanceIterator{};
	template<typename O, typename HashIt, typename ValueIt>
	struct templated_iterator : std::iterator<std::forward_iterator_tag, O, ptrdiff_t>
	{
		templated_iterator()
			: hash_it(), hash_end(), value_it()
		{
		}
		templated_iterator(HashIt hash_it, HashIt hash_end, ValueIt value_it, AdvanceIterator)
			: hash_it(hash_it), hash_end(hash_end), value_it(value_it)
		{
			while (this->hash_it != this->hash_end && this->hash_it->hash == detail::empty_hash)
			{
				++this->hash_it;
				++this->value_it;
			}
		}
		templated_iterator(HashIt hash_it, HashIt hash_end, ValueIt value_it, DoNotAdvanceIterator)
			: hash_it(hash_it), hash_end(hash_end), value_it(value_it)
		{
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
			return *this = templated_iterator(hash_it + ptrdiff_t(1), hash_end, value_it + ptrdiff_t(1), AdvanceIterator{});
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
		friend class fat_sherwood_map;
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
				std::fill(hash_begin, hash_end, HashAndDistance{ detail::empty_hash, 0, 0 });
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
	inline void init_empty(value_alloc & alloc, hash_pointer hash_it, value_pointer value_it, HashAndDistance hash_and_distance, Args &&... args)
	{
		value_allocator_traits::construct(alloc, std::addressof(*value_it), std::forward<Args>(args)...);
		*hash_it = hash_and_distance;
		++_size;
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
			storage_iterator(typename fat_sherwood_map::iterator it)
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
			*it.hash_it = HashAndDistance { detail::empty_hash, 0, 0 };
		}

		void clear()
		{
			for (auto it = begin(), end = this->end(); it != end; ++it)
			{
				if (it.hash_it->hash != detail::empty_hash) iterator_destroy(it);
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

		iterator erase(iterator first, iterator last)
		{
			if (first == last) return first;
			size_t capacity = this->capacity();
			do
			{
				iterator initial = initial_bucket(first.hash_it->hash, capacity);
				--initial.hash_it->bucket_size;
				WrapAroundIt wrap(first, begin(), end());
				for (WrapAroundIt next = wrap;; wrap = next)
				{
					++next;
					size_t hash = next.it.hash_it->hash;
					if (hash == detail::empty_hash || next.it.hash_it->bucket_distance == 0)
					{
						break;
					}
					*wrap.it.value_it = std::move(*next.it.value_it);
					wrap.it.hash_it->hash = hash;
					if (next.it == last) --last;
				}
				for (WrapAroundIt next_bucket = std::next(WrapAroundIt(initial, begin(), end())); next_bucket.it.hash_it->bucket_distance > 0; ++next_bucket)
				{
					--next_bucket.it.hash_it->bucket_distance;
				}
				iterator_destroy(wrap.it);
				while (first.hash_it->hash == detail::empty_hash)
				{
					++first;
					if (first == end()) return first;
				}
			}
			while (first != end() && first != last);
			return first;
		}

		iterator initial_bucket(size_type hash, size_t capacity)
		{
			return begin() + hash % capacity;
		}
		const_iterator initial_bucket(size_type hash, size_t capacity) const
		{
			return begin() + hash % capacity;
		}

		template<typename First>
		inline iterator find_hash(size_type hash, const First & first)
		{
			if (this->hash_begin == this->hash_end)
			{
				return end();
			}
			iterator initial = initial_bucket(hash, this->capacity());
			distance_type bucket_size = initial.hash_it->bucket_size;
			if (bucket_size == 0)
				return end();
			WrapAroundIt it{initial, begin(), end()};
			for (std::advance(it, initial.hash_it->bucket_distance); bucket_size--; ++it)
			{
				if (static_cast<KeyOrValueEquality &>(*this)(it.it.value_it->first, first))
					return it.it;
			}
			return end();
		}
		enum EmplacePosResultType
		{
			FoundEqual,
			FoundNotEqual
		};
		struct EmplacePosResult
		{
			iterator bucket_it;
			iterator insert_it;
			EmplacePosResultType result;
		};


		template<typename First>
		EmplacePosResult find_emplace_pos(size_type hash, const First & first)
		{
			if (this->hash_begin == this->hash_end)
			{
				return { end(), end(), FoundNotEqual };
			}
			iterator initial = initial_bucket(hash, this->capacity());
			WrapAroundIt it{initial, begin(), end()};
			std::advance(it, initial.hash_it->bucket_distance);
			for (distance_type i = 0; i < initial.hash_it->bucket_size; ++i, ++it)
			{
				if (it.it.hash_it->hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it.value_it->first, first))
					return { initial, it.it, FoundEqual };
			}
			return { initial, it.it, FoundNotEqual };
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
	fat_sherwood_map() = default;
	fat_sherwood_map(const fat_sherwood_map & other)
		: fat_sherwood_map(other, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator()))
	{
	}
	fat_sherwood_map(const fat_sherwood_map & other, const Allocator & alloc)
		: entries(detail::next_prime(detail::required_capacity(other.size(), other.max_load_factor())), other.hash_function(), other.key_eq(), alloc)
		, _max_load_factor(other._max_load_factor)
	{
		insert(other.begin(), other.end());
	}
	fat_sherwood_map(fat_sherwood_map && other) noexcept(std::is_nothrow_move_constructible<StorageType>::value)
		: entries(std::move(other.entries))
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	fat_sherwood_map(fat_sherwood_map && other, const Allocator & alloc)
		: entries(std::move(other.entries), alloc)
		, _size(other._size)
		, _max_load_factor(other._max_load_factor)
	{
		other._size = 0;
	}
	fat_sherwood_map & operator=(const fat_sherwood_map & other)
	{
		fat_sherwood_map(other).swap(*this);
		return *this;
	}
	fat_sherwood_map & operator=(fat_sherwood_map && other) = default;
	fat_sherwood_map & operator=(std::initializer_list<value_type> il)
	{
		fat_sherwood_map(il).swap(*this);
		return *this;
	}
	explicit fat_sherwood_map(size_t bucket_count, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, allocator)
	{
	}
	explicit fat_sherwood_map(const Allocator & allocator)
		: fat_sherwood_map(0, allocator)
	{
	}
	explicit fat_sherwood_map(size_t bucket_count, const Allocator & allocator)
		: fat_sherwood_map(bucket_count, hasher(), allocator)
	{
	}
	fat_sherwood_map(size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: fat_sherwood_map(bucket_count, hash, key_equal(), allocator)
	{
	}
	template<typename It>
	fat_sherwood_map(It begin, It end, size_t bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: entries(bucket_count, hash, equality, allocator)
	{
		insert(begin, end);
	}
	template<typename It>
	fat_sherwood_map(It begin, It end, size_t bucket_count, const Allocator & allocator)
		: fat_sherwood_map(begin, end, bucket_count, hasher(), allocator)
	{
	}
	template<typename It>
	fat_sherwood_map(It begin, It end, size_t bucket_count, const hasher & hash, const Allocator & allocator)
		: fat_sherwood_map(begin, end, bucket_count, hash, key_equal(), allocator)
	{
	}
	fat_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count = 0, const hasher & hash = hasher(), const key_equal & equality = key_equal(), const Allocator & allocator = Allocator())
		: fat_sherwood_map(il.begin(), il.end(), bucket_count, hash, equality, allocator)
	{
	}
	fat_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const hasher & hash, const Allocator & allocator)
		: fat_sherwood_map(il, bucket_count, hash, key_equal(), allocator)
	{
	}
	fat_sherwood_map(std::initializer_list<value_type> il, size_type bucket_count, const Allocator & allocator)
		: fat_sherwood_map(il, bucket_count, hasher(), allocator)
	{
	}

	iterator begin()				{ return { entries.hash_begin,	entries.hash_end, entries.value_begin,	AdvanceIterator{}		}; }
	iterator end()					{ return { entries.hash_end,	entries.hash_end, entries.value_end(),	DoNotAdvanceIterator{}	}; }
	const_iterator begin()	const	{ return { entries.hash_begin,	entries.hash_end, entries.value_begin,	AdvanceIterator{}		}; }
	const_iterator end()	const	{ return { entries.hash_end,	entries.hash_end, entries.value_end(),	DoNotAdvanceIterator{}	}; }
	const_iterator cbegin()	const	{ return { entries.hash_begin,	entries.hash_end, entries.value_begin,	AdvanceIterator{}		}; }
	const_iterator cend()	const	{ return { entries.hash_end,	entries.hash_end, entries.value_end(),	DoNotAdvanceIterator{}	}; }

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
		return emplace_with_hash(static_cast<KeyOrValueHasher &>(entries)(first), std::forward<First>(first), std::forward<Args>(args)...);
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
		auto erased = entries.erase(iterator_const_cast(pos), iterator_const_cast(std::next(pos)));
		return { erased.hash_it, entries.hash_end, erased.value_it, AdvanceIterator{} };
	}
	iterator erase(const_iterator first, const_iterator last)
	{
		_size -= std::distance(first, last);
		auto erased = entries.erase(iterator_const_cast(first), iterator_const_cast(last));
		return { erased.hash_it, entries.hash_end, erased.value_it, AdvanceIterator{} };
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
		return { found.hash_it, entries.hash_end, found.value_it, DoNotAdvanceIterator{} };
	}
	template<typename T>
	const_iterator find(const T & key) const
	{
		return const_cast<fat_sherwood_map &>(*this).find(key);
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
		return const_cast<fat_sherwood_map &>(*this).at(key);
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
		return const_cast<fat_sherwood_map &>(*this).equal_range(key);
	}
	void swap(fat_sherwood_map & other)
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
	bool operator==(const fat_sherwood_map & other) const
	{
		if (size() != other.size()) return false;
		return std::all_of(begin(), end(), [&other](const value_type & value)
		{
			auto found = other.find(value.first);
			return found != other.end() && *found == value;
		});
	}
	bool operator!=(const fat_sherwood_map & other) const
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
	template<typename First, typename... Args>
	std::pair<iterator, bool> emplace_with_hash(size_t hash, First && first, Args &&... args)
	{
		auto found = entries.find_emplace_pos(hash, first);
		if (found.result == StorageType::FoundEqual)
		{
			return { { found.insert_it.hash_it, entries.hash_end, found.insert_it.value_it, DoNotAdvanceIterator{} }, false };
		}
		if (size() + 1 > _max_load_factor * entries.capacity())
		{
			grow();
			found = entries.find_emplace_pos(hash, first);
		}
		typedef typename StorageType::WrapAroundIt WrapIt;
		if (found.insert_it.hash_it->hash == detail::empty_hash)
		{
			if (found.insert_it == found.bucket_it)
			{
				init_empty(entries, found.insert_it.hash_it, found.insert_it.value_it, HashAndDistance{ hash, 0, 1 }, std::forward<First>(first), std::forward<Args>(args)...);
			}
			else
			{
				init_empty(entries, found.insert_it.hash_it, found.insert_it.value_it, HashAndDistance{ hash, 1, 0 }, std::forward<First>(first), std::forward<Args>(args)...);
				++found.bucket_it.hash_it->bucket_size;
				for (WrapIt bucket_wrap = ++WrapIt(found.bucket_it, entries.begin(), entries.end()); bucket_wrap.it != found.insert_it; ++bucket_wrap)
				{
					++bucket_wrap.it.hash_it->bucket_distance;
				}
			}
			return { { found.insert_it.hash_it, entries.hash_end, found.insert_it.value_it, DoNotAdvanceIterator{} }, true };
		}
		value_type new_value(std::forward<First>(first), std::forward<Args>(args)...);
		auto insert_wrap = WrapIt(found.insert_it, entries.begin(), entries.end());
		value_type & current_value = *found.insert_it.value_it;
		size_t & current_hash = found.insert_it.hash_it->hash;
		for (auto bucket_wrap = std::next(WrapIt(found.bucket_it, entries.begin(), entries.end())); ; ++bucket_wrap)
		{
			++bucket_wrap.it.hash_it->bucket_distance;
			distance_type bucket_size = bucket_wrap.it.hash_it->bucket_size;
			if (!bucket_size) continue;
			std::advance(insert_wrap, bucket_size);
			if (insert_wrap.it.hash_it->hash == detail::empty_hash)
			{
				for (++bucket_wrap; bucket_wrap != insert_wrap; ++bucket_wrap)
				{
					++bucket_wrap.it.hash_it->bucket_distance;
				}
				init_empty(entries, insert_wrap.it.hash_it, insert_wrap.it.value_it, HashAndDistance{ current_hash, 1, 0 }, std::move(current_value));
				break;
			}
			std::iter_swap(found.insert_it.value_it, insert_wrap.it.value_it);
			std::swap(current_hash, insert_wrap.it.hash_it->hash);
		}
		++found.bucket_it.hash_it->bucket_size;
		current_value = std::move(new_value);
		current_hash = hash;
		return { { found.insert_it.hash_it, entries.hash_end, found.insert_it.value_it, DoNotAdvanceIterator{} }, true };
	}
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
			size_t hash = it->hash;
			if (hash != detail::empty_hash)
				emplace_with_hash(hash, std::move(*value_it));
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

	template<typename>
	friend struct sherwood_test;
};

