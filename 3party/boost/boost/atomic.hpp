#ifndef BOOST_ATOMIC_HPP
#define BOOST_ATOMIC_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>

#include <boost/memory_order.hpp>
#include <boost/atomic/platform.hpp>
#include <boost/atomic/detail/base.hpp>
#include <boost/atomic/detail/integral-casts.hpp>

namespace boost {

template<typename T>
class atomic : public detail::atomic::internal_atomic<T> {
public:
	typedef detail::atomic::internal_atomic<T> super;

	atomic() {}
	explicit atomic(T v) : super(v) {}
private:
	atomic(const atomic &);
	void operator=(const atomic &);
};


template<>
class atomic<bool> : private detail::atomic::internal_atomic<bool> {
public:
	typedef detail::atomic::internal_atomic<bool> super;

	atomic() {}
	explicit atomic(bool v) : super(v) {}

	using super::load;
	using super::store;
	using super::compare_exchange_strong;
	using super::compare_exchange_weak;
	using super::exchange;
	using super::is_lock_free;

	operator bool(void) const volatile {return load();}
	bool operator=(bool v) volatile {store(v); return v;}
private:
	atomic(const atomic &);
	void operator=(const atomic &);
};

template<>
class atomic<void *> : private detail::atomic::internal_atomic<void *, sizeof(void *), int> {
public:
	typedef detail::atomic::internal_atomic<void *, sizeof(void *), int> super;

	atomic() {}
	explicit atomic(void * p) : super(p) {}
	using super::load;
	using super::store;
	using super::compare_exchange_strong;
	using super::compare_exchange_weak;
	using super::exchange;
	using super::is_lock_free;

	operator void *(void) const volatile {return load();}
	void * operator=(void * v) volatile {store(v); return v;}

private:
	atomic(const atomic &);
	void * operator=(const atomic &);
};

/* FIXME: pointer arithmetic still missing */

template<typename T>
class atomic<T *> : private detail::atomic::internal_atomic<intptr_t> {
public:
	typedef detail::atomic::internal_atomic<intptr_t> super;

	atomic() {}
	explicit atomic(T * p) : super((intptr_t)p) {}

	T *load(memory_order order=memory_order_seq_cst) const volatile
	{
		return (T*)super::load(order);
	}
	void store(T *v, memory_order order=memory_order_seq_cst) volatile
	{
		super::store((intptr_t)v, order);
	}
	bool compare_exchange_strong(
		T * &expected,
		T * desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return compare_exchange_strong(expected, desired, order, detail::atomic::calculate_failure_order(order));
	}
	bool compare_exchange_weak(
		T * &expected,
		T *desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return compare_exchange_weak(expected, desired, order, detail::atomic::calculate_failure_order(order));
	}
	bool compare_exchange_weak(
		T * &expected,
		T *desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		intptr_t expected_=(intptr_t)expected, desired_=(intptr_t)desired;
		bool success=super::compare_exchange_weak(expected_, desired_, success_order, failure_order);
		expected=(T*)expected_;
		return success;
	}
	bool compare_exchange_strong(
		T * &expected,
		T *desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		intptr_t expected_=(intptr_t)expected, desired_=(intptr_t)desired;
		bool success=super::compare_exchange_strong(expected_, desired_, success_order, failure_order);
		expected=(T*)expected_;
		return success;
	}
	T *exchange(T * replacement, memory_order order=memory_order_seq_cst) volatile
	{
		return (T*)super::exchange((intptr_t)replacement, order);
	}
	using super::is_lock_free;

	operator T *(void) const volatile {return load();}
	T * operator=(T * v) volatile {store(v); return v;}

	T * fetch_add(ptrdiff_t diff, memory_order order=memory_order_seq_cst) volatile
	{
		return (T*)super::fetch_add(diff*sizeof(T), order);
	}
	T * fetch_sub(ptrdiff_t diff, memory_order order=memory_order_seq_cst) volatile
	{
		return (T*)super::fetch_sub(diff*sizeof(T), order);
	}

	T *operator++(void) volatile {return fetch_add(1)+1;}
	T *operator++(int) volatile {return fetch_add(1);}
	T *operator--(void) volatile {return fetch_sub(1)-1;}
	T *operator--(int) volatile {return fetch_sub(1);}
private:
	atomic(const atomic &);
	T * operator=(const atomic &);
};

class atomic_flag : private atomic<int> {
public:
	typedef atomic<int> super;
	using super::is_lock_free;

	atomic_flag(bool initial_state) : super(initial_state?1:0) {}
	atomic_flag() {}

	bool test_and_set(memory_order order=memory_order_seq_cst)
	{
		return super::exchange(1, order) != 0;
	}
	void clear(memory_order order=memory_order_seq_cst)
	{
		super::store(0, order);
	}
};

typedef atomic<char> atomic_char;
typedef atomic<unsigned char> atomic_uchar;
typedef atomic<signed char> atomic_schar;
typedef atomic<uint8_t> atomic_uint8_t;
typedef atomic<int8_t> atomic_int8_t;
typedef atomic<unsigned short> atomic_ushort;
typedef atomic<short> atomic_short;
typedef atomic<uint16_t> atomic_uint16_t;
typedef atomic<int16_t> atomic_int16_t;
typedef atomic<unsigned int> atomic_uint;
typedef atomic<int> atomic_int;
typedef atomic<uint32_t> atomic_uint32_t;
typedef atomic<int32_t> atomic_int32_t;
typedef atomic<unsigned long> atomic_ulong;
typedef atomic<long> atomic_long;
typedef atomic<uint64_t> atomic_uint64_t;
typedef atomic<int64_t> atomic_int64_t;
typedef atomic<unsigned long long> atomic_ullong;
typedef atomic<long long> atomic_llong;
typedef atomic<void*> atomic_address;
typedef atomic<bool> atomic_bool;

static inline void atomic_thread_fence(memory_order order)
{
	detail::atomic::platform_atomic_thread_fence<memory_order>(order);
}

}

#endif
