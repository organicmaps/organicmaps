/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_INFINITY_HPP_JOFA_100322
#define BOOST_ICL_TYPE_TRAITS_INFINITY_HPP_JOFA_100322

#include <string>
#include <boost/static_assert.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/mpl/if.hpp>

namespace boost{ namespace icl
{

#ifdef BOOST_MSVC 
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif                        

    template <class Type> struct numeric_infinity
    {
        typedef numeric_infinity type;

        static Type value()
        {
            BOOST_STATIC_ASSERT((is_numeric<Type>::value));
            if(std::numeric_limits<Type>::has_infinity)
                return std::numeric_limits<Type>::infinity();
            else
                return (std::numeric_limits<Type>::max)();
        }
    };

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


    template <class Type> struct infinity
    {
        typedef infinity type;

        static Type value()
        {
            return
            mpl::if_<is_numeric<Type>,
                     numeric_infinity<Type>,
                     identity_element<Type> >::type::value();
        }
    };

}} // namespace boost icl

#endif


