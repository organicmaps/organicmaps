/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_DELETE_HPP
#define BOOST_ALIGN_ALIGNED_DELETE_HPP

/**
 Class aligned_delete.

 @file
 @author Glen Fernandes
*/

#include <boost/config.hpp>
#include <boost/align/aligned_alloc.hpp>
#include <boost/align/aligned_delete_forward.hpp>

/**
 Boost namespace.
*/
namespace boost {
    /**
     Alignment namespace.
    */
    namespace alignment {
        /**
         Class aligned_delete.
        */
        class aligned_delete {
        public:
            /**
             Calls `~T()` on `ptr` to destroy the object and then
             calls `aligned_free` on `ptr` to free the allocated
             memory.

             @remark **Note:** If `T` is an incomplete type, the
               program is ill-formed.
            */
            template<class T>
            void operator()(T* ptr) const
                BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(ptr->~T())) {
                if (ptr) {
                    ptr->~T();
                    alignment::aligned_free(ptr);
                }
            }
        };
    }
}

#endif
