///////////////////////////////////////////////////////////////////////////////
/// \file ignore_unused.hpp
/// Definintion of ignore_unused, a dummy function for suppressing compiler
/// warnings
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_DETAIL_IGNORE_UNUSED_HPP_EAN_03_03_2008
#define BOOST_PROTO_DETAIL_IGNORE_UNUSED_HPP_EAN_03_03_2008

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename T>
        inline void ignore_unused(T const &)
        {}
    }
}}

#endif
