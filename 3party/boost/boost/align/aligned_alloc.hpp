/*
 Copyright (c) 2014 Glen Joseph Fernandes
 glenfe at live dot com

 Distributed under the Boost Software License,
 Version 1.0. (See accompanying file LICENSE_1_0.txt
 or copy at http://boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGNED_ALLOC_HPP
#define BOOST_ALIGN_ALIGNED_ALLOC_HPP

/**
 Functions aligned_alloc and aligned_free.

 @file
 @author Glen Fernandes
*/

#include <boost/config.hpp>

/**
 @cond
*/
#if defined(BOOST_HAS_UNISTD_H)
#include <unistd.h>
#endif

#if defined(__APPLE__) || defined(__APPLE_CC__) || defined(macintosh)
#include <AvailabilityMacros.h>
#endif

#if defined(_MSC_VER)
#include <boost/align/detail/aligned_alloc_msvc.hpp>
#elif defined(__MINGW32__) && (__MSVCRT_VERSION__ >= 0x0700)
#include <boost/align/detail/aligned_alloc_msvc.hpp>
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#include <boost/align/detail/aligned_alloc_posix.hpp>
#elif MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
#include <boost/align/detail/aligned_alloc_macos.hpp>
#elif defined(__ANDROID__)
#include <boost/align/detail/aligned_alloc_android.hpp>
#elif defined(__SunOS_5_11) || defined(__SunOS_5_12)
#include <boost/align/detail/aligned_alloc_posix.hpp>
#elif defined(sun) || defined(__sun)
#include <boost/align/detail/aligned_alloc_sunos.hpp>
#elif (_POSIX_C_SOURCE >= 200112L) || (_XOPEN_SOURCE >= 600)
#include <boost/align/detail/aligned_alloc_posix.hpp>
#else
#include <boost/align/detail/aligned_alloc.hpp>
#endif
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
         Allocates space for an object whose alignment is
         specified by `alignment`, whose size is
         specified by `size`, and whose value is
         indeterminate.

         @param alignment Shall be a power of two.

         @param size Size of space to allocate.

         @return A null pointer or a pointer to the
           allocated space.

         @remark **Note:** On certain platforms, the
           alignment may be rounded up to `alignof(void*)`
           and the space allocated may be slightly larger
           than `size` bytes, by an additional
           `sizeof(void*)` and `alignment - 1` bytes.
        */
        inline void* aligned_alloc(std::size_t alignment,
            std::size_t size) BOOST_NOEXCEPT;

        /**
         Causes the space pointed to by `ptr` to be
         deallocated, that is, made available for further
         allocation. If `ptr` is a null pointer, no
         action occurs. Otherwise, if the argument does
         not match a pointer earlier returned by the
         `aligned_alloc` function, or if the space has
         been deallocated by a call to `aligned_free`,
         the behavior is undefined.
        */
        inline void aligned_free(void* ptr)
            BOOST_NOEXCEPT;
    }
}

#endif
