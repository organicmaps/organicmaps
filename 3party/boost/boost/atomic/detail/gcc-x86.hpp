#ifndef BOOST_DETAIL_ATOMIC_GCC_X86_HPP
#define BOOST_DETAIL_ATOMIC_GCC_X86_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/atomic/detail/base.hpp>
#include <boost/atomic/detail/builder.hpp>

namespace boost {
namespace detail {
namespace atomic {

static inline void fence_before(memory_order order)
{
	switch(order) {
		case memory_order_consume:
		case memory_order_release:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

static inline void fence_after(memory_order order)
{
	switch(order) {
		case memory_order_acquire:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

static inline void full_fence(void)
{
#if defined(__amd64__)
			__asm__ __volatile__("mfence" ::: "memory");
#else
			/* could use mfence iff i686, but it does not appear to matter much */
			__asm__ __volatile__("lock; addl $0, (%%esp)"  ::: "memory");
#endif
}

static inline void fence_after_load(memory_order order)
{
	switch(order) {
		case memory_order_seq_cst:
			full_fence();
		case memory_order_acquire:
		case memory_order_acq_rel:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

template<>
inline void platform_atomic_thread_fence(memory_order order)
{
	switch(order) {
		case memory_order_seq_cst:
			full_fence();
		case memory_order_acquire:
		case memory_order_consume:
		case memory_order_acq_rel:
		case memory_order_release:
			__asm__ __volatile__ ("" ::: "memory");
		default:;
	}
}

template<typename T>
class atomic_x86_8 {
public:
	explicit atomic_x86_8(T v) : i(v) {}
	atomic_x86_8() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		if (order!=memory_order_seq_cst) {
			fence_before(order);
			*reinterpret_cast<volatile T *>(&i)=v;
		} else {
			exchange(v);
		}
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		fence_before(success_order);
		T prev=expected;
		__asm__ __volatile__("lock; cmpxchgb %1, %2\n" : "=a" (prev) : "q" (desired), "m" (i), "a" (expected) : "memory");
		bool success=(prev==expected);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		expected=prev;
		return success;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T r, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("xchgb %0, %1\n" : "=q" (r) : "m"(i), "0" (r) : "memory");
		return r;
	}
	T fetch_add(T c, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("lock; xaddb %0, %1" : "+q" (c), "+m" (i) :: "memory");
		return c;
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;
};

template<typename T>
class platform_atomic_integral<T, 1> : public build_atomic_from_add<atomic_x86_8<T> > {
public:
	typedef build_atomic_from_add<atomic_x86_8<T> > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

template<typename T>
class atomic_x86_16 {
public:
	explicit atomic_x86_16(T v) : i(v) {}
	atomic_x86_16() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		if (order!=memory_order_seq_cst) {
			fence_before(order);
			*reinterpret_cast<volatile T *>(&i)=v;
		} else {
			exchange(v);
		}
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		fence_before(success_order);
		T prev=expected;
		__asm__ __volatile__("lock; cmpxchgw %1, %2\n" : "=a" (prev) : "q" (desired), "m" (i), "a" (expected) : "memory");
		bool success=(prev==expected);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		expected=prev;
		return success;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T r, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("xchgw %0, %1\n" : "=r" (r) : "m"(i), "0" (r) : "memory");
		return r;
	}
	T fetch_add(T c, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("lock; xaddw %0, %1" : "+r" (c), "+m" (i) :: "memory");
		return c;
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;
};

template<typename T>
class platform_atomic_integral<T, 2> : public build_atomic_from_add<atomic_x86_16<T> > {
public:
	typedef build_atomic_from_add<atomic_x86_16<T> > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

template<typename T>
class atomic_x86_32 {
public:
	explicit atomic_x86_32(T v) : i(v) {}
	atomic_x86_32() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		if (order!=memory_order_seq_cst) {
			fence_before(order);
			*reinterpret_cast<volatile T *>(&i)=v;
		} else {
			exchange(v);
		}
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		fence_before(success_order);
		T prev=expected;
		__asm__ __volatile__("lock; cmpxchgl %1, %2\n" : "=a" (prev) : "q" (desired), "m" (i), "a" (expected) : "memory");
		bool success=(prev==expected);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		expected=prev;
		return success;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T r, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("xchgl %0, %1\n" : "=r" (r) : "m"(i), "0" (r) : "memory");
		return r;
	}
	T fetch_add(T c, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("lock; xaddl %0, %1" : "+r" (c), "+m" (i) :: "memory");
		return c;
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;
};

template<typename T>
class platform_atomic_integral<T, 4> : public build_atomic_from_add<atomic_x86_32<T> > {
public:
	typedef build_atomic_from_add<atomic_x86_32<T> > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};

#if defined(__amd64__)
template<typename T>
class atomic_x86_64 {
public:
	explicit atomic_x86_64(T v) : i(v) {}
	atomic_x86_64() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=*reinterpret_cast<volatile const T *>(&i);
		fence_after_load(order);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		if (order!=memory_order_seq_cst) {
			fence_before(order);
			*reinterpret_cast<volatile T *>(&i)=v;
		} else {
			exchange(v);
		}
	}
	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		fence_before(success_order);
		T prev=expected;
		__asm__ __volatile__("lock; cmpxchgq %1, %2\n" : "=a" (prev) : "q" (desired), "m" (i), "a" (expected) : "memory");
		bool success=(prev==expected);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		expected=prev;
		return success;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T r, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("xchgq %0, %1\n" : "=r" (r) : "m"(i), "0" (r) : "memory");
		return r;
	}
	T fetch_add(T c, memory_order order=memory_order_seq_cst) volatile
	{
		__asm__ __volatile__("lock; xaddq %0, %1" : "+r" (c), "+m" (i) :: "memory");
		return c;
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;
} __attribute__((aligned(8)));

#elif defined(__i686__)

template<typename T>
class atomic_x86_64 {
private:
	typedef atomic_x86_64 this_type;
public:
	explicit atomic_x86_64(T v) : i(v) {}
	atomic_x86_64() {}

	bool compare_exchange_strong(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		long scratch;
		fence_before(success_order);
		T prev=expected;
		/* Make sure ebx is saved and restored properly in case
		this object is compiled as "position independent". Since
		programmers on x86 tend to forget specifying -DPIC or
		similar, always assume PIC.

		To make this work uniformly even in the non-PIC case,
		setup register constraints such that ebx can not be
		used by accident e.g. as base address for the variable
		to be modified. Accessing "scratch" should always be okay,
		as it can only be placed on the stack (and therefore
		accessed through ebp or esp only).

		In theory, could push/pop ebx onto/off the stack, but movs
		to a prepared stack slot turn out to be faster. */
		__asm__ __volatile__(
			"movl %%ebx, %1\n"
			"movl %2, %%ebx\n"
			"lock; cmpxchg8b 0(%4)\n"
			"movl %1, %%ebx\n"
			: "=A" (prev), "=m" (scratch)
			: "D" ((long)desired), "c" ((long)(desired>>32)), "S" (&i), "0" (prev)
			: "memory");
		bool success=(prev==expected);
		if (success) fence_after(success_order);
		else fence_after(failure_order);
		expected=prev;
		return success;
	}
	bool compare_exchange_weak(
		T &expected,
		T desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		return compare_exchange_strong(expected, desired, success_order, failure_order);
	}
	T exchange(T r, memory_order order=memory_order_seq_cst) volatile
	{
		T prev=i;
		do {} while(!compare_exchange_strong(prev, r, order, memory_order_relaxed));
		return prev;
	}

	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		/* this is a bit problematic -- there is no other
		way to atomically load a 64 bit value, but of course
		compare_exchange requires write access to the memory
		area */
		T expected=i;
		do { } while(!const_cast<this_type *>(this)->compare_exchange_strong(expected, expected, order, memory_order_relaxed));
		return expected;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		exchange(v, order);
	}
	T fetch_add(T c, memory_order order=memory_order_seq_cst) volatile
	{
		T expected=i, desired;;
		do {
			desired=expected+c;
		} while(!compare_exchange_strong(expected, desired, order, memory_order_relaxed));
		return expected;
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;
} __attribute__((aligned(8))) ;

#endif

#if defined(__amd64__) || defined(__i686__)
template<typename T>
class platform_atomic_integral<T, 8> : public build_atomic_from_add<atomic_x86_64<T> >{
public:
	typedef build_atomic_from_add<atomic_x86_64<T> > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};
#endif

}
}
}

#endif
