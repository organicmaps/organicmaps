/*-----------------------------------------------------------------------------+
Copyright (c) 2008-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_TYPE_TRAITS_DIFFERENCE_TYPE_OF_HPP_JOFA_080911
#define BOOST_ICL_TYPE_TRAITS_DIFFERENCE_TYPE_OF_HPP_JOFA_080911

#include <boost/config.hpp> // For macro BOOST_STATIC_CONSTANT
#include <boost/type_traits/is_pointer.hpp>
#include <boost/mpl/or.hpp>
#include <boost/icl/type_traits/no_type.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>

namespace boost{ namespace icl
{
    //--------------------------------------------------------------------------
    template<class Type>
    struct has_difference
    {
        typedef has_difference type;
        BOOST_STATIC_CONSTANT(bool, 
            value = (mpl::or_< is_numeric<Type>
                             , is_pointer<Type> >::value) 
            );
    };

    //--------------------------------------------------------------------------
    template <class Type, bool has_difference> 
    struct get_difference_type;

    template <class Type>
    struct get_difference_type<Type, false>
    {
        typedef no_type type;
    };

    template <class Type>
    struct get_difference_type<Type*, true>
    {
        typedef std::ptrdiff_t type;
    };

    template <class Type>
    struct get_difference_type<Type, true>
    {
        typedef Type type;
    };

    //--------------------------------------------------------------------------
    template<class Type> 
    struct difference_type_of
    { 
        typedef typename 
            get_difference_type<Type, has_difference<Type>::value>::type type;
    };

}} // namespace boost icl

#endif


