/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNMENT_OF_HPP
#define BOOST_ALIGN_ALIGNMENT_OF_HPP

/**
 Class template alignment_of.

 @file
 @author Glen Fernandes
*/

#include <boost/config.hpp>
#include <boost/align/alignment_of_forward.hpp>
#include <boost/align/detail/type_traits.hpp>

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <boost/align/detail/alignment_of_cxx11.hpp>
#elif defined(BOOST_MSVC)
#include <boost/align/detail/alignment_of_msvc.hpp>
#elif defined(BOOST_CLANG)
#include <boost/align/detail/alignment_of_clang.hpp>
#elif defined(__ghs__) && (__GHS_VERSION_NUMBER >= 600)
#include <boost/align/detail/alignment_of_gcc.hpp>
#elif defined(__CODEGEARC__)
#include <boost/align/detail/alignment_of_codegear.hpp>
#elif defined(__GNUC__) && defined(__unix__) && !defined(__LP64__)
#include <boost/align/detail/alignment_of.hpp>
#elif __GNUC__ > 4
#include <boost/align/detail/alignment_of_gcc.hpp>
#elif (__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)
#include <boost/align/detail/alignment_of_gcc.hpp>
#else
#include <boost/align/detail/alignment_of.hpp>
#endif

/**
 Boost namespace.
*/
namespace boost {
    /**
     Alignment namespace.
    */
    namespace alignment {
        /**
         Class template alignment_of.

         @remark **Value:** `alignof(T)`.
        */
        template<class T>
        struct alignment_of {
            /**
             @enum
            */
            enum {
                /**
                 @cond
                */
                value = detail::alignment_of<typename
                    detail::remove_cv<typename
                    detail::remove_all_extents<typename
                    detail::remove_reference<T>::
                    type>::type>::type>::value
                /**
                 @endcond
                */
            };
        };
    }
}

#endif
