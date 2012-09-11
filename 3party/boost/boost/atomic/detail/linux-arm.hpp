#ifndef BOOST_DETAIL_ATOMIC_LINUX_ARM_HPP
#define BOOST_DETAIL_ATOMIC_LINUX_ARM_HPP

//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  Copyright (c) 2009 Helge Bahmann
//  Copyright (c) 2009 Phil Endecott
//  ARM Code by Phil Endecott, based on other architectures.

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/base.hpp>
#include <boost/atomic/detail/builder.hpp>

namespace boost {
namespace detail {
namespace atomic {


// Different ARM processors have different atomic instructions.  In particular,
// architecture versions before v6 (which are still in widespread use, e.g. the
// Intel/Marvell XScale chips like the one in the NSLU2) have only atomic swap.
// On Linux the kernel provides some support that lets us abstract away from
// these differences: it provides emulated CAS and barrier functions at special
// addresses that are garaunteed not to be interrupted by the kernel.  Using
// this facility is slightly slower than inline assembler would be, but much
// faster than a system call.
//
// For documentation, see arch/arm/kernel/entry-armv.S in the kernel source
// (search for "User Helpers").


typedef void (kernel_dmb_t)(void);
#define BOOST_ATOMIC_KERNEL_DMB (*(kernel_dmb_t *)0xffff0fa0)

static inline void fence_before(memory_order order)
{
	switch(order) {
		// FIXME I really don't know which of these cases should call
		// kernel_dmb() and which shouldn't...
		case memory_order_consume:
		case memory_order_release:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			BOOST_ATOMIC_KERNEL_DMB();
		default:;
	}
}

static inline void fence_after(memory_order order)
{
	switch(order) {
		// FIXME I really don't know which of these cases should call
		// kernel_dmb() and which shouldn't...
		case memory_order_acquire:
		case memory_order_acq_rel:
		case memory_order_seq_cst:
			BOOST_ATOMIC_KERNEL_DMB();
		default:;
	}
}

#undef BOOST_ATOMIC_KERNEL_DMB


template<typename T>
class atomic_linux_arm_4 {

	typedef int (kernel_cmpxchg_t)(T oldval, T newval, T *ptr);
#	define BOOST_ATOMIC_KERNEL_CMPXCHG (*(kernel_cmpxchg_t *)0xffff0fc0)
	// Returns 0 if *ptr was changed.

public:
	explicit atomic_linux_arm_4(T v) : i(v) {}
	atomic_linux_arm_4() {}
	T load(memory_order order=memory_order_seq_cst) const volatile
	{
		T v=const_cast<volatile const T &>(i);
		fence_after(order);
		return v;
	}
	void store(T v, memory_order order=memory_order_seq_cst) volatile
	{
		fence_before(order);
		const_cast<volatile T &>(i)=v;
	}
	bool compare_exchange_strong(
                T &expected,
                T desired,
                memory_order success_order,
                memory_order failure_order) volatile
        {
		// Aparently we can consider kernel_cmpxchg to be strong if it is retried
		// by the kernel after being interrupted, which I think it is.
		// Also it seems that when an ll/sc implementation is used the kernel
		// loops until the store succeeds.
		bool success = BOOST_ATOMIC_KERNEL_CMPXCHG(expected,desired,&i)==0;
		if (!success) e = load(memory_order_relaxed);
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
	T exchange(T replacement, memory_order order=memory_order_seq_cst) volatile
	{
		// Copied from build_exchange.
                T o=load(memory_order_relaxed);
                do {} while(!compare_exchange_weak(o, replacement, order));
                return o;
		// Note that ARM has an atomic swap instruction that we could use here:
		//   T oldval;
		//   asm volatile ("swp\t%0, %1, [%2]" : "=&r"(oldval) : "r" (replacement), "r" (&i) : "memory");
		//   return oldval;
		// This instruction is deprecated in architecture >= 6.  I'm unsure how inefficient
		// its implementation is on those newer architectures.  I don't think this would gain
		// much since exchange() is not used often.
	}

	bool is_lock_free(void) const volatile {return true;}
protected:
	typedef T integral_type;
private:
	T i;

#	undef BOOST_ATOMIC_KERNEL_CMPXCHG

};

template<typename T>
class platform_atomic_integral<T, 4> : public build_atomic_from_exchange<atomic_linux_arm_4<T> > {
public:
	typedef build_atomic_from_exchange<atomic_linux_arm_4<T> > super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};


template<typename T>
class platform_atomic_integral<T, 1> : public build_atomic_from_larger_type<atomic_linux_arm_4<uint32_t>, T > {
public:
	typedef build_atomic_from_larger_type<atomic_linux_arm_4<uint32_t>, T> super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};


template<typename T>
class platform_atomic_integral<T, 2> : public build_atomic_from_larger_type<atomic_linux_arm_4<uint32_t>, T > {
public:
	typedef build_atomic_from_larger_type<atomic_linux_arm_4<uint32_t>, T> super;
	explicit platform_atomic_integral(T v) : super(v) {}
	platform_atomic_integral(void) {}
};


typedef atomic_linux_arm_4<void *> platform_atomic_address;


}
}
}

#endif
