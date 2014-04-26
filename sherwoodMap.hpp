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
constexpr const size_t special_value = 0;
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

	template<typename It>
	struct WrappingIterator
	{
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

	static size_t adjust_for_special_value(size_t hash)
	{
		return std::max(size_t(1), hash);
	}
	static constexpr bool is_empty(size_t hash)
	{
		return hash == detail::special_value;
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
			while (this->hash_it != this->hash_end && is_empty(*this->hash_it))
			{
				++this->hash_it;
				++this->value_it;
			}
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

		template<typename OO, typename OHashIt, typename OValueIt>
		operator templated_iterator<OO, OHashIt, OValueIt>() const
		{
			return { hash_it, hash_end, value_it };
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
				std::fill(hash_begin, hash_end, detail::special_value);
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
		struct read_storage_iterator : std::iterator<std::random_access_iterator_tag, value_type >
		{
			read_storage_iterator(HashIt hash_it, ValueIt value_it)
				: hash_it(hash_it), value_it(value_it)
			{
			}
			read_storage_iterator(iterator it)
				: hash_it(it.hash_it), value_it(it.value_it)
			{
			}
			value_type & operator*() const
			{
				return *value_it;
			}
			value_type & operator->() const
			{
				return &*value_it;
			}
			read_storage_iterator & operator++()
			{
				++hash_it;
				++value_it;
				return *this;
			}
			read_storage_iterator operator++(int)
			{
				read_storage_iterator before = *this;
				++*this;
				return before;
			}
			read_storage_iterator & operator--()
			{
				--hash_it;
				--value_it;
				return *this;
			}
			read_storage_iterator operator--(int)
			{
				read_storage_iterator before = *this;
				--*this;
				return before;
			}
			read_storage_iterator & operator+=(ptrdiff_t distance)
			{
				hash_it += distance;
				value_it += distance;
				return *this;
			}
			read_storage_iterator & operator-=(ptrdiff_t distance)
			{
				hash_it -= distance;
				value_it -= distance;
				return *this;
			}
			ptrdiff_t operator-(const read_storage_iterator & other) const
			{
				return hash_it - other.hash_it;
			}
			read_storage_iterator operator-(ptrdiff_t distance) const
			{
				return { hash_it - distance, value_it - distance };
			}
			read_storage_iterator operator+(ptrdiff_t distance) const
			{
				return { hash_it + distance, value_it + distance };
			}
			bool operator==(const read_storage_iterator & other) const
			{
				return hash_it == other.hash_it;
			}
			bool operator!=(const read_storage_iterator & other) const
			{
				return !(*this == other);
			}
			bool operator<(const read_storage_iterator & other) const
			{
				return hash_it < other.hash_it;
			}
			bool operator<=(const read_storage_iterator & other) const
			{
				return !(other < *this);
			}
			bool operator>(const read_storage_iterator & other) const
			{
				return other < *this;
			}
			bool operator>=(const read_storage_iterator & other) const
			{
				return !(*this < other);
			}
			template<typename OHashIt, typename OValueIt>
			operator read_storage_iterator<OHashIt, OValueIt>()
			{
				return { hash_it, value_it };
			}
			template<typename OHashIt, typename OValueIt>
			operator read_storage_iterator<OHashIt, OValueIt>() const
			{
				return { hash_it, value_it };
			}

			HashIt hash_it;
			ValueIt value_it;
		};

		struct AssignEmptyProxy {};
		template<typename It>
		struct AssignProxyValue;
		template<typename It>
		struct AssignProxyIt
		{
			AssignProxyIt & operator=(AssignProxyValue<It> && other)
			{
				assign(other.hash, std::move(other.value));
				return *this;
			}
			AssignProxyIt & operator=(AssignProxyIt && other)
			{
				assign(*other.it.it.hash_it, std::move(*other.it.it.value_it));
				return *this;
			}
			AssignProxyIt & operator=(AssignEmptyProxy)
			{
				if (!is_empty(*it.it.hash_it))
				{
					value_allocator_traits::destroy(*it.alloc, it.it.value_it);
					*it.it.hash_it = detail::special_value;
				}
				return *this;
			}
			void assign(size_t hash, value_type && value)
			{
				if (is_empty(hash))
				{
					*this = AssignEmptyProxy{};
				}
				else if (is_empty(*it.it.hash_it))
				{
					init_empty(*it.alloc, it.it.hash_it, it.it.value_it, hash, std::move(value));
				}
				else
				{
					*it.it.value_it = std::move(value);
					*it.it.hash_it = hash;
				}
			}
			friend void swap(AssignProxyIt && lhs, AssignProxyIt && rhs)
			{
				AssignProxyValue<It> temp = std::move(lhs);
				lhs = std::move(rhs);
				rhs = std::move(temp);
			}

			It it;
		};
		template<typename It>
		struct AssignProxyValue
		{
			AssignProxyValue(AssignProxyIt<It> && it)
				: hash(*it.it.it.hash_it)
			{
				if (!is_empty(hash)) new (&value) value_type(std::move(*it.it.it.value_it));
			}
			AssignProxyValue(AssignProxyValue && other)
				: hash(other.hash)
			{
				if (!is_empty(hash)) new (&value) value_type(std::move(other.value));
			}
			AssignProxyValue(const AssignProxyValue & other)
				: hash(other.hash)
			{
				if (!is_empty(hash)) new (&value) value_type(other.value);
			}
			~AssignProxyValue()
			{
				if (!is_empty(hash)) value.~pair();
			}

			size_t hash;
			union
			{
				char unused;
				value_type value;
			};
		};
		template<typename HashIt, typename ValueIt>
		struct write_storage_iterator : std::iterator<std::random_access_iterator_tag, AssignProxyValue<write_storage_iterator<HashIt, ValueIt> > >
		{
			write_storage_iterator(read_storage_iterator<HashIt, ValueIt> it, value_alloc * alloc)
				: it(it), alloc(alloc)
			{
			}
			AssignProxyIt<write_storage_iterator> operator*()
			{
				return { *this };
			}
			write_storage_iterator & operator=(AssignEmptyProxy)
			{
			}
			write_storage_iterator & operator++()
			{
				++it;
				return *this;
			}
			write_storage_iterator operator++(int)
			{
				write_storage_iterator before = *this;
				++*this;
				return before;
			}
			write_storage_iterator & operator--()
			{
				--it;
				return *this;
			}
			write_storage_iterator operator--(int)
			{
				write_storage_iterator before = *this;
				--*this;
				return before;
			}
			write_storage_iterator & operator+=(ptrdiff_t distance)
			{
				it += distance;
				return *this;
			}
			write_storage_iterator & operator-=(ptrdiff_t distance)
			{
				it -= distance;
				return *this;
			}
			ptrdiff_t operator-(const write_storage_iterator & other) const
			{
				return it - other.it;
			}
			write_storage_iterator operator-(ptrdiff_t distance) const
			{
				return { it - distance, alloc };
			}
			write_storage_iterator operator+(ptrdiff_t distance) const
			{
				return { it + distance, alloc };
			}
			bool operator==(const write_storage_iterator & other) const
			{
				return it == other.it;
			}
			bool operator!=(const write_storage_iterator & other) const
			{
				return !(*this == other);
			}
			bool operator<(const write_storage_iterator & other) const
			{
				return it < other.it;
			}
			bool operator<=(const write_storage_iterator & other) const
			{
				return !(other < *this);
			}
			bool operator>(const write_storage_iterator & other) const
			{
				return other < *this;
			}
			bool operator>=(const write_storage_iterator & other) const
			{
				return !(*this < other);
			}

			read_storage_iterator<HashIt, ValueIt> it;
			value_alloc * alloc;
		};

		typedef write_storage_iterator<hash_pointer, value_pointer> write_iterator;
		typedef read_storage_iterator<hash_pointer, value_pointer> read_iterator;
		typedef read_storage_iterator<const_hash_pointer, const_value_pointer> const_read_iterator;

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
		void clear()
		{
			size_t capacity_copy = this->capacity();
			for (size_t i = 0; i < capacity_copy; ++i)
			{
				size_t & hash = this->hash_begin[i];
				if (!is_empty(hash))
				{
					value_allocator_traits::destroy(*this, &this->value_begin[i]);
					hash = detail::special_value;
				}
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

		read_iterator erase(read_iterator first, read_iterator last)
		{
			write_iterator first_write{ first, this };
			write_iterator last_write{ last, this };
			std::fill(first_write, last_write, AssignEmptyProxy{});
			write_iterator stop{ find_erase_stop_bucket(last), this };
			if (stop < last_write)
			{
				write_iterator end = { read_iterator{ this->hash_end, value_begin + this->capacity() }, this };
				std::rotate(first_write, last_write, end);
				difference_type num_to_move = last - first;
				write_iterator begin{ read_begin(), this };
				write_iterator middle = std::min(stop, begin + num_to_move);
				std::swap_ranges(begin, middle, end - num_to_move);
				std::rotate(begin, middle, stop);
			}
			else std::rotate(first_write, last_write, stop);
			return first;
		}
		read_iterator initial_bucket(size_type hash, size_t capacity)
		{
			return read_begin() + hash % capacity;
		}
		const_read_iterator initial_bucket(size_type hash, size_t capacity) const
		{
			return read_begin() + hash % capacity;
		}
		size_t distance_to_initial_bucket(read_iterator it, size_t hash, size_t capacity) const
		{
			typename hash_allocator_traits::const_pointer initial = initial_bucket(hash, capacity).hash_it;
			if (const_read_iterator(it).hash_it < initial) return (typename hash_allocator_traits::const_pointer(this->hash_end) - initial) + (it.hash_it - this->hash_begin);
			else return const_read_iterator(it).hash_it - initial;
		}

		enum FindResult
		{
			FoundEmpty,
			FoundEqual,
			FoundNotEqual
		};

		typedef WrappingIterator<typename StorageType::read_iterator> WrapAroundIt;

		template<typename First>
		std::pair<read_iterator, FindResult> find_hash(size_type hash, const First & first)
		{
			size_t capacity_copy = this->capacity();
			if (!capacity_copy)
			{
				read_iterator end{ this->hash_end, value_end() };
				return { end, FoundNotEqual };
			}
			size_t distance = 0;
			for (WrapAroundIt it{initial_bucket(hash, capacity_copy), read_begin(), read_end()};; ++it, ++distance)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				size_t current_hash = *it.it.hash_it;
				if (is_empty(current_hash))
					return { it.it, FoundEmpty };
				if (current_hash == hash && static_cast<KeyOrValueEquality &>(*this)(it.it.value_it->first, first))
					return { it.it, FoundEqual };
				if (distance_to_initial_bucket(it.it, current_hash, capacity_copy) < distance)
					return { it.it, FoundNotEqual };
			}
		}
		template<typename First>
		std::pair<const_read_iterator, FindResult> find_hash(size_type hash, const First & first) const
		{
			return const_cast<sherwood_map &>(*this).find_hash(hash, first);
		}

		value_pointer value_begin;
		value_pointer value_end() const
		{
			return {};
		}

		read_iterator read_begin()					{ return { this->hash_begin,	value_begin }; }
		read_iterator read_end()					{ return { this->hash_end,		value_end() }; }
		const_read_iterator read_begin()	const	{ return { this->hash_begin,	value_begin }; }
		const_read_iterator read_end()		const	{ return { this->hash_end,		value_end() }; }
		read_iterator write_begin()					{ return { this->hash_begin,	value_begin, this }; }
		read_iterator write_end()					{ return { this->hash_end,		value_end(), this }; }

		read_iterator find_erase_stop_bucket(read_iterator start)
		{
			size_t capacity_copy = this->capacity();
			if (start == read_end())
				start = read_begin();
			for (WrapAroundIt it{start, read_begin(), read_end()};; ++it)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				size_t hash = *it.it.hash_it;
				if (is_empty(hash) || distance_to_initial_bucket(it.it, hash, capacity_copy) == 0)
					return it.it;
			}
		}
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
			throw std::logic_error("should be impossible to get here because I already handled the FoundEqual case at the beginning of the function");
		case StorageType::FoundEmpty:
			init_empty(entries, found.first.hash_it, found.first.value_it, hash, std::forward<First>(first), std::forward<Args>(args)...);
			++_size;
			return { { found.first.hash_it, entries.hash_end, found.first.value_it }, true };
		case StorageType::FoundNotEqual:
			size_t capacity = entries.capacity();
			size_t & current_hash = *found.first.hash_it;
			value_type & current_value = *found.first.value_it;
			size_t distance = entries.distance_to_initial_bucket(found.first, current_hash, capacity);
			typename StorageType::WrapAroundIt next{found.first, entries.read_begin(), entries.read_end()};
			for (++distance, ++next; !is_empty(*next.it.hash_it); ++distance, ++next)
			{
				#ifdef PROFILE_SHERWOOD_MAP
					++detail::profile_cost;
				#endif
				size_t next_distance = entries.distance_to_initial_bucket(next.it, *next.it.hash_it, capacity);
				if (next_distance < distance)
				{
					distance = next_distance;
					std::iter_swap(found.first.value_it, next.it.value_it);
					std::iter_swap(found.first.hash_it, next.it.hash_it);
				}
			}
			init_empty(entries, next.it.hash_it, next.it.value_it, current_hash, std::move(current_value));
			current_value = value_type(std::forward<First>(first), std::forward<Args>(args)...);
			current_hash = hash;
			++_size;
			return { { found.first.hash_it, entries.hash_end, found.first.value_it }, true };
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
		auto non_const_pos = iterator_const_cast(pos);
		auto next = non_const_pos;
		auto erased = entries.erase(non_const_pos, ++next);
		return { erased.hash_it, entries.hash_end, entries.value_begin };
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
		if (new_size > entries.capacity()) reallocate(detail::next_prime(new_size));
	}
	void rehash(size_type count)
	{
		if (count < size() / max_load_factor()) count = detail::next_prime(size_type(std::ceil(size() / max_load_factor())));
		reallocate(count);
	}
	size_type bucket(const key_type & key) const
	{
		return entries.initial_bucket(static_cast<const KeyOrValueHasher &>(entries)(key), entries.capacity()) - entries.read_begin();
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
		reallocate(detail::next_prime(std::max(required_capacity(_size + 1, _max_load_factor), entries.capacity() * 2)));
	}
	void reallocate(size_type size)
	{
		StorageType replacement(size, entries.hash_function(), entries.key_eq(), entries.get_allocator());
		entries.swap(replacement);
		_size = 0;
		auto value_it = replacement.value_begin;
		for (auto it = replacement.hash_begin, end = replacement.hash_end; it != end; ++it, ++value_it)
		{
			if (!is_empty(*it))
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
			iterator non_const;
			const_iterator it;
		};
		static_assert(sizeof(iterator) == sizeof(const_iterator), "doing reinterpret_cast here");
		return caster(it).non_const;
	}

	static size_type required_capacity(size_type size, float load_factor)
	{
		return size_type(std::ceil(size / load_factor));
	}

	StorageType entries;
	size_t _size = 0;
	static constexpr const float default_load_factor = 0.85f;
	float _max_load_factor = default_load_factor;
};
