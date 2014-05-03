#pragma once

#include <algorithm>
#include <cstddef>

namespace detail
{
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
}
