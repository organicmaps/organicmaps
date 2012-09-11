#ifndef BOOST_DETAIL_ATOMIC_VALID_INTEGRAL_TYPES_HPP
#define BOOST_DETAIL_ATOMIC_VALID_INTEGRAL_TYPES_HPP

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/cstdint.hpp>

namespace boost {
namespace detail {
namespace atomic {

template<typename T> struct is_integral_type {typedef void test;};

template<> struct is_integral_type<char> {typedef int test;};

template<> struct is_integral_type<unsigned char> {typedef int test;};
template<> struct is_integral_type<signed char> {typedef int test;};
template<> struct is_integral_type<unsigned short> {typedef int test;};
template<> struct is_integral_type<signed short> {typedef int test;};
template<> struct is_integral_type<unsigned int> {typedef int test;};
template<> struct is_integral_type<signed int> {typedef int test;};
template<> struct is_integral_type<unsigned long> {typedef int test;};
template<> struct is_integral_type<long> {typedef int test;};
#ifdef BOOST_HAS_LONG_LONG
template<> struct is_integral_type<unsigned long long> {typedef int test;};
template<> struct is_integral_type<signed long long> {typedef int test;};
#endif

}
}
}

#endif
