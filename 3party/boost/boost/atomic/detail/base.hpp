#ifndef BOOST_DETAIL_ATOMIC_BASE_HPP
#define BOOST_DETAIL_ATOMIC_BASE_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/detail/fallback.hpp>
#include <boost/atomic/detail/builder.hpp>
#include <boost/atomic/detail/valid_integral_types.hpp>

namespace boost {
namespace detail {
namespace atomic {

static inline memory_order calculate_failure_order(memory_order order)
{
	switch(order) {
		case memory_order_acq_rel: return memory_order_acquire;
		case memory_order_release: return memory_order_relaxed;
		default: return order;
	}
}

template<typename T, unsigned short Size=sizeof(T)>
class platform_atomic : public fallback_atomic<T> {
public:
	typedef fallback_atomic<T> super;

	explicit platform_atomic(T v) : super(v) {}
	platform_atomic() {}
protected:
	typedef typename super::integral_type integral_type;
};

template<typename T, unsigned short Size=sizeof(T)>
class platform_atomic_integral : public build_atomic_from_exchange<fallback_atomic<T> > {
public:
	typedef build_atomic_from_exchange<fallback_atomic<T> > super;

	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral() {}
protected:
	typedef typename super::integral_type integral_type;
};

template<typename T>
static inline void platform_atomic_thread_fence(T order)
{
	/* FIXME: this does not provide
	sequential consistency, need one global
	variable for that... */
	platform_atomic<int> a;
	a.exchange(0, order);
}

template<typename T, unsigned short Size=sizeof(T), typename Int=typename is_integral_type<T>::test>
class internal_atomic;

template<typename T, unsigned short Size>
class internal_atomic<T, Size, void> : private detail::atomic::platform_atomic<T> {
public:
	typedef detail::atomic::platform_atomic<T> super;

	internal_atomic() {}
	explicit internal_atomic(T v) : super(v) {}

	operator T(void) const volatile {return load();}
	T operator=(T v) volatile {store(v); return v;}

	using super::is_lock_free;
	using super::load;
	using super::store;
	using super::exchange;

	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return super::compare_exchange_strong(expected, desired, order, calculate_failure_order(order));
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return super::compare_exchange_strong(expected, desired, order, calculate_failure_order(order));
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return super::compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return super::compare_exchange_strong(expected, desired, success_order, failure_order);
	}
private:
	internal_atomic(const internal_atomic &);
	void operator=(const internal_atomic &);
};

template<typename T, unsigned short Size>
class internal_atomic<T, Size, int> : private detail::atomic::platform_atomic_integral<T> {
public:
	typedef detail::atomic::platform_atomic_integral<T> super;
	typedef typename super::integral_type integral_type;

	internal_atomic() {}
	explicit internal_atomic(T v) : super(v) {}

	using super::is_lock_free;
	using super::load;
	using super::store;
	using super::exchange;
	using super::fetch_add;
	using super::fetch_sub;
	using super::fetch_and;
	using super::fetch_or;
	using super::fetch_xor;

	operator integral_type(void) const volatile {return load();}
	integral_type operator=(integral_type v) volatile {store(v); return v;}

	integral_type operator&=(integral_type c) volatile {return fetch_and(c)&c;}
	integral_type operator|=(integral_type c) volatile {return fetch_or(c)|c;}
	integral_type operator^=(integral_type c) volatile {return fetch_xor(c)^c;}

	integral_type operator+=(integral_type c) volatile {return fetch_add(c)+c;}
	integral_type operator-=(integral_type c) volatile {return fetch_sub(c)-c;}

	integral_type operator++(void) volatile {return fetch_add(1)+1;}
	integral_type operator++(int) volatile {return fetch_add(1);}
	integral_type operator--(void) volatile {return fetch_sub(1)-1;}
	integral_type operator--(int) volatile {return fetch_sub(1);}

	bool compare_exchange_strong(
		integral_type &expected,
		integral_type desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return super::compare_exchange_strong(expected, desired, order, calculate_failure_order(order));
	}
	bool compare_exchange_weak(
		integral_type &expected,
		integral_type desired,
		memory_order order=memory_order_seq_cst) volatile
	{
		return super::compare_exchange_strong(expected, desired, order, calculate_failure_order(order));
	}
	bool compare_exchange_strong(
		integral_type &expected,
		integral_type desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return super::compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	bool compare_exchange_weak(
		integral_type &expected,
		integral_type desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return super::compare_exchange_strong(expected, desired, success_order, failure_order);
	}
private:
	internal_atomic(const internal_atomic &);
	void operator=(const internal_atomic &);
};

}
}
}

#endif
