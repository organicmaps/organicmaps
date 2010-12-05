///////////////////////////////////////////////////////////////////////////////
/// \file as_lvalue.hpp
/// Contains definition the as_lvalue() functions.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_AS_LVALUE_HPP_EAN_12_27_2007
#define BOOST_PROTO_TRANSFORM_AS_LVALUE_HPP_EAN_12_27_2007

#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename T>
        T &as_lvalue(T &t)
        {
            return t;
        }

        template<typename T>
        T const &as_lvalue(T const &t)
        {
            return t;
        }
    }
}}

#endif
