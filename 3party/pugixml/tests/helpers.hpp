#ifndef HEADER_TEST_HELPERS_HPP
#define HEADER_TEST_HELPERS_HPP

#include "common.hpp"

#include <utility>

template <typename T> static void generic_bool_ops_test(const T& obj)
{
	T null;

	CHECK(!null);
	CHECK(obj);
	CHECK(!!obj);

#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' (performance warning) - we really want to just cast to bool instead of !!
#endif

	bool b1 = null, b2 = obj;

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

	CHECK(!b1);
	CHECK(b2);

	CHECK(obj && b2);
	CHECK(obj || b2);
	CHECK(obj && obj);
	CHECK(obj || obj);
}

template <typename T> static void generic_eq_ops_test(const T& obj1, const T& obj2)
{
	T null = T();

	// operator==
	CHECK(null == null);
	CHECK(obj1 == obj1);
	CHECK(!(null == obj1));
	CHECK(!(null == obj2));
	CHECK(T(null) == null);
	CHECK(T(obj1) == obj1);

	// operator!=
	CHECK(!(null != null));
	CHECK(!(obj1 != obj1));
	CHECK(null != obj1);
	CHECK(null != obj2);
	CHECK(!(T(null) != null));
	CHECK(!(T(obj1) != obj1));
}

template <typename T> static void generic_rel_ops_test(T obj1, T obj2)
{
	T null = T();

	// obj1 < obj2 (we use operator<, but there is no other choice
	if (obj1 > obj2)
	{
		T temp = obj1;
		obj1 = obj2;
		obj2 = temp;
	}

	// operator<
	CHECK(null < obj1);
	CHECK(null < obj2);
	CHECK(obj1 < obj2);
	CHECK(!(null < null));
	CHECK(!(obj1 < obj1));
	CHECK(!(obj1 < null));
	CHECK(!(obj2 < obj1));

	// operator<=
	CHECK(null <= obj1);
	CHECK(null <= obj2);
	CHECK(obj1 <= obj2);
	CHECK(null <= null);
	CHECK(obj1 <= obj1);
	CHECK(!(obj1 <= null));
	CHECK(!(obj2 <= obj1));

	// operator>
	CHECK(obj1 > null);
	CHECK(obj2 > null);
	CHECK(obj2 > obj1);
	CHECK(!(null > null));
	CHECK(!(obj1 > obj1));
	CHECK(!(null > obj1));
	CHECK(!(obj1 > obj2));

	// operator>=
	CHECK(obj1 >= null);
	CHECK(obj2 >= null);
	CHECK(obj2 >= obj1);
	CHECK(null >= null);
	CHECK(obj1 >= obj1);
	CHECK(!(null >= obj1));
	CHECK(!(obj1 >= obj2));
}

template <typename T> static void generic_empty_test(const T& obj)
{
	T null;

	CHECK(null.empty());
	CHECK(!obj.empty());
}

#endif
