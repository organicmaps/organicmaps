/*
(c) 2014 Glen Joseph Fernandes
glenjofe at gmail dot com

Distributed under the Boost Software
License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef BOOST_ALIGN_DETAIL_INTEGRAL_CONSTANT_HPP
#define BOOST_ALIGN_DETAIL_INTEGRAL_CONSTANT_HPP

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>
#endif

namespace boost {
namespace alignment {
namespace detail {

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
using std::integral_constant;
#else
template<class T, T Value>
struct integral_constant {
    typedef T value_type;
    typedef integral_constant<T, Value> type;

#if !defined(BOOST_NO_CXX11_CONSTEXPR)
    constexpr operator value_type() const {
        return Value;
    }

    static constexpr T value = Value;
#else
    enum {
        value = Value
    };
#endif
};
#endif

} /* :detail */
} /* :alignment */
} /* :boost */

#endif
