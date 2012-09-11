#ifndef BOOST_DETAIL_ATOMIC_FALLBACK_HPP
#define BOOST_DETAIL_ATOMIC_FALLBACK_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <string.h>
#include <boost/smart_ptr/detail/spinlock_pool.hpp>

namespace boost {
namespace detail {
namespace atomic {

template<typename T>
class fallback_atomic {
public:
	fallback_atomic(void) {}
	explicit fallback_atomic(const T &t) {memcpy(&i, &t, sizeof(T));}

	void store(const T &t, memory_order order=memory_order_seq_cst) volatile
	{
		detail::spinlock_pool<0>::scoped_lock guard(const_cast<T*>(&i));
		memcpy((void*)&i, &t, sizeof(T));
	}
	T load(memory_order /*order*/=memory_order_seq_cst) volatile const
	{
		detail::spinlock_pool<0>::scoped_lock guard(const_cast<T*>(&i));
		T tmp;
		memcpy(&tmp, (T*)&i, sizeof(T));
		return tmp;
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order /*success_order*/,
		memory_order /*failure_order*/) volatile
	{
		detail::spinlock_pool<0>::scoped_lock guard(const_cast<T*>(&i));
		if (memcmp((void*)&i, &expected, sizeof(T))==0) {
			memcpy((void*)&i, &desired, sizeof(T));
			return true;
		} else {
			memcpy(&expected, (void*)&i, sizeof(T));
			return false;
		}
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T replacement, memory_order /*order*/=memory_order_seq_cst) volatile
	{
		detail::spinlock_pool<0>::scoped_lock guard(const_cast<T*>(&i));
		T tmp;
		memcpy(&tmp, (void*)&i, sizeof(T));
		memcpy((void*)&i, &replacement, sizeof(T));
		return tmp;
	}
	bool is_lock_free(void) const volatile {return false;}
protected:
	T i;
	typedef T integral_type;
};

}
}
}

#endif
