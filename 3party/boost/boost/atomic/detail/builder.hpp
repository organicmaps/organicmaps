#ifndef BOOST_DETAIL_ATOMIC_BUILDER_HPP
#define BOOST_DETAIL_ATOMIC_BUILDER_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/endian.hpp>
#include <boost/atomic/detail/valid_integral_types.hpp>

namespace boost {
namespace detail {
namespace atomic {

/*
given a Base that implements:

- load(memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)

generates exchange and compare_exchange_strong
*/
template<typename Base>
class build_exchange : public Base {
public:
	typedef typename Base::integral_type integral_type;

	using Base::load;
	using Base::compare_exchange_weak;

	bool compare_exchange_strong(
		integral_type &expected,
		integral_type desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		integral_type expected_save=expected;
		while(true) {
			if (compare_exchange_weak(expected, desired, success_order, failure_order)) return true;
			if (expected_save!=expected) return false;
			expected=expected_save;
		}
	}

	integral_type exchange(integral_type replacement, memory_order order=memory_order_seq_cst) volatile
	{
		integral_type o=load(memory_order_relaxed);
		do {} while(!compare_exchange_weak(o, replacement, order, memory_order_relaxed));
		return o;
	}

	build_exchange() {}
	explicit build_exchange(integral_type i) : Base(i) {}
};

/*
given a Base that implements:

- fetch_add_var(integral_type c, memory_order order)
- fetch_inc(memory_order order)
- fetch_dec(memory_order order)

creates a fetch_add method that delegates to fetch_inc/fetch_dec if operand
is constant +1/-1, and uses fetch_add_var otherwise

the intention is to allow optimizing the incredibly common case of +1/-1
*/
template<typename Base>
class build_const_fetch_add : public Base {
public:
	typedef typename Base::integral_type integral_type;

	integral_type fetch_add(
		integral_type c,
		memory_order order=memory_order_seq_cst) volatile
	{
		if (__builtin_constant_p(c)) {
			switch(c) {
				case -1: return fetch_dec(order);
				case 1: return fetch_inc(order);
			}
		}
		return fetch_add_var(c, order);
	}

	build_const_fetch_add() {}
	explicit build_const_fetch_add(integral_type i) : Base(i) {}
protected:
	using Base::fetch_add_var;
	using Base::fetch_inc;
	using Base::fetch_dec;
};

/*
given a Base that implements:

- load(memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)

generates a -- not very efficient, but correct -- fetch_add operation
*/
template<typename Base>
class build_fetch_add : public Base {
public:
	typedef typename Base::integral_type integral_type;

	using Base::compare_exchange_weak;

	integral_type fetch_add(
		integral_type c, memory_order order=memory_order_seq_cst) volatile
	{
		integral_type o=Base::load(memory_order_relaxed), n;
		do {n=o+c;} while(!compare_exchange_weak(o, n, order, memory_order_relaxed));
		return o;
	}

	build_fetch_add() {}
	explicit build_fetch_add(integral_type i) : Base(i) {}
};

/*
given a Base that implements:

- fetch_add(integral_type c, memory_order order)

generates fetch_sub and post/pre- increment/decrement operators
*/
template<typename Base>
class build_arithmeticops : public Base {
public:
	typedef typename Base::integral_type integral_type;

	using Base::fetch_add;

	integral_type fetch_sub(
		integral_type c,
		memory_order order=memory_order_seq_cst) volatile
	{
		return fetch_add(-c, order);
	}

	build_arithmeticops() {}
	explicit build_arithmeticops(integral_type i) : Base(i) {}
};

/*
given a Base that implements:

- load(memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)

generates -- not very efficient, but correct -- fetch_and, fetch_or and fetch_xor operators
*/
template<typename Base>
class build_logicops : public Base {
public:
	typedef typename Base::integral_type integral_type;

	using Base::compare_exchange_weak;
	using Base::load;

	integral_type fetch_and(integral_type c, memory_order order=memory_order_seq_cst) volatile
	{
		integral_type o=load(memory_order_relaxed), n;
		do {n=o&c;} while(!compare_exchange_weak(o, n, order, memory_order_relaxed));
		return o;
	}
	integral_type fetch_or(integral_type c, memory_order order=memory_order_seq_cst) volatile
	{
		integral_type o=load(memory_order_relaxed), n;
		do {n=o|c;} while(!compare_exchange_weak(o, n, order, memory_order_relaxed));
		return o;
	}
	integral_type fetch_xor(integral_type c, memory_order order=memory_order_seq_cst) volatile
	{
		integral_type o=load(memory_order_relaxed), n;
		do {n=o^c;} while(!compare_exchange_weak(o, n, order, memory_order_relaxed));
		return o;
	}

	build_logicops() {}
	build_logicops(integral_type i) : Base(i) {}
};

/*
given a Base that implements:

- load(memory_order order)
- store(integral_type i, memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)

generates the full set of atomic operations for integral types
*/
template<typename Base>
class build_atomic_from_minimal : public build_logicops< build_arithmeticops< build_fetch_add< build_exchange<Base> > > > {
public:
	typedef build_logicops< build_arithmeticops< build_fetch_add< build_exchange<Base> > > > super;
	typedef typename super::integral_type integral_type;

	build_atomic_from_minimal(void) {}
	build_atomic_from_minimal(typename super::integral_type i) : super(i) {}
};

/*
given a Base that implements:

- load(memory_order order)
- store(integral_type i, memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)
- compare_exchange_strong(integral_type &expected, integral_type desired, memory_order order)
- exchange(integral_type replacement, memory_order order)
- fetch_add_var(integral_type c, memory_order order)
- fetch_inc(memory_order order)
- fetch_dec(memory_order order)

generates the full set of atomic operations for integral types
*/
template<typename Base>
class build_atomic_from_typical : public build_logicops< build_arithmeticops< build_const_fetch_add<Base> > > {
public:
	typedef build_logicops< build_arithmeticops< build_const_fetch_add<Base> > > super;
	typedef typename super::integral_type integral_type;

	build_atomic_from_typical(void) {}
	build_atomic_from_typical(typename super::integral_type i) : super(i) {}
};

/*
given a Base that implements:

- load(memory_order order)
- store(integral_type i, memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)
- compare_exchange_strong(integral_type &expected, integral_type desired, memory_order order)
- exchange(integral_type replacement, memory_order order)
- fetch_add(integral_type c, memory_order order)

generates the full set of atomic operations for integral types
*/
template<typename Base>
class build_atomic_from_add : public build_logicops< build_arithmeticops<Base> > {
public:
	typedef build_logicops< build_arithmeticops<Base> > super;
	typedef typename super::integral_type integral_type;

	build_atomic_from_add(void) {}
	build_atomic_from_add(typename super::integral_type i) : super(i) {}
};

/*
given a Base that implements:

- load(memory_order order)
- store(integral_type i, memory_order order)
- compare_exchange_weak(integral_type &expected, integral_type desired, memory_order order)
- compare_exchange_strong(integral_type &expected, integral_type desired, memory_order order)
- exchange(integral_type replacement, memory_order order)

generates the full set of atomic operations for integral types
*/
template<typename Base>
class build_atomic_from_exchange : public build_logicops< build_arithmeticops< build_fetch_add<Base> > > {
public:
	typedef build_logicops< build_arithmeticops< build_fetch_add<Base> > > super;
	typedef typename super::integral_type integral_type;

	build_atomic_from_exchange(void) {}
	build_atomic_from_exchange(typename super::integral_type i) : super(i) {}
};


/*
given a Base that implements:

- compare_exchange_weak()

generates load, store and compare_exchange_weak for a smaller
data type (e.g. an atomic "byte" embedded into a temporary
and properly aligned atomic "int").
*/
template<typename Base, typename Type>
class build_base_from_larger_type {
public:
	typedef Type integral_type;

	build_base_from_larger_type() {}
	build_base_from_larger_type(integral_type t) {store(t, memory_order_relaxed);}

	integral_type load(memory_order order=memory_order_seq_cst) const volatile
	{
		larger_integral_type v=get_base().load(order);
		return extract(v);
	}
	bool compare_exchange_weak(integral_type &expected,
		integral_type desired,
		memory_order success_order,
		memory_order failure_order) volatile
	{
		larger_integral_type expected_;
		larger_integral_type desired_;

		expected_=get_base().load(memory_order_relaxed);
		expected_=insert(expected_, expected);
		desired_=insert(expected_, desired);
		bool success=get_base().compare_exchange_weak(expected_, desired_, success_order, failure_order);
		expected=extract(expected_);
		return success;
	}
	void store(integral_type v,
		memory_order order=memory_order_seq_cst) volatile
	{
		larger_integral_type expected, desired;
		expected=get_base().load(memory_order_relaxed);
		do {
			desired=insert(expected, v);
		} while(!get_base().compare_exchange_weak(expected, desired, order, memory_order_relaxed));
	}

	bool is_lock_free(void)
	{
		return get_base().is_lock_free();
	}
private:
	typedef typename Base::integral_type larger_integral_type;

	const Base &get_base(void) const volatile
	{
		intptr_t address=(intptr_t)this;
		address&=~(sizeof(larger_integral_type)-1);
		return *reinterpret_cast<const Base *>(address);
	}
	Base &get_base(void) volatile
	{
		intptr_t address=(intptr_t)this;
		address&=~(sizeof(larger_integral_type)-1);
		return *reinterpret_cast<Base *>(address);
	}
	unsigned int get_offset(void) const volatile
	{
		intptr_t address=(intptr_t)this;
		address&=(sizeof(larger_integral_type)-1);
		return address;
	}

	unsigned int get_shift(void) const volatile
	{
#if defined(BOOST_LITTLE_ENDIAN)
		return get_offset()*8;
#elif  defined(BOOST_BIG_ENDIAN)
		return (sizeof(larger_integral_type)-sizeof(integral_type)-get_offset())*8;
#else
	#error "Unknown endian"
#endif
	}

	integral_type extract(larger_integral_type v) const volatile
	{
		return v>>get_shift();
	}

	larger_integral_type insert(larger_integral_type target, integral_type source) const volatile
	{
		larger_integral_type tmp=source;
		larger_integral_type mask=(larger_integral_type)-1;

		mask=~(mask<<(8*sizeof(integral_type)));

		mask=mask<<get_shift();
		tmp=tmp<<get_shift();

		tmp=(tmp & mask) | (target & ~mask);

		return tmp;
	}

	integral_type i;
};

/*
given a Base that implements:

- compare_exchange_weak()

generates the full set of atomic ops for a smaller
data type (e.g. an atomic "byte" embedded into a temporary
and properly aligned atomic "int").
*/
template<typename Base, typename Type>
class build_atomic_from_larger_type : public build_atomic_from_minimal< build_base_from_larger_type<Base, Type> > {
public:
	typedef build_atomic_from_minimal< build_base_from_larger_type<Base, Type> > super;
	//typedef typename super::integral_type integral_type;
	typedef Type integral_type;

	build_atomic_from_larger_type() {}
	build_atomic_from_larger_type(integral_type v) : super(v) {}
};

}
}
}

#endif
