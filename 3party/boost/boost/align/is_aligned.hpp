/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_IS_ALIGNED_HPP
#define BOOST_ALIGN_IS_ALIGNED_HPP

/**
 Function is_aligned.

 @file
 @author Glen Fernandes
*/

/**
 @cond
*/
#include <boost/align/detail/is_aligned.hpp>
/**
 @endcond
*/

/**
 Boost namespace.
*/
namespace boost {
    /**
     Alignment namespace.
    */
    namespace alignment {
        /**
         Determines whether the space pointed to by
         `ptr` has alignment specified by
         `alignment`.

         @param alignment Shall be a power of two.

         @param ptr Pointer to test for alignment.

         @return `true` if and only if `ptr` points
           to space that has alignment specified by
           `alignment`.
        */
        inline bool is_aligned(std::size_t alignment,
            const void* ptr) BOOST_NOEXCEPT;
    }
}

#endif
