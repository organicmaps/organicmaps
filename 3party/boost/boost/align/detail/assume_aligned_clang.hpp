/*
(c) 2015 Glen Joseph Fernandes
glenjofe at gmail dot com

Distributed under the Boost Software
License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef BOOST_ALIGN_DETAIL_ASSUME_ALIGNED_CLANG_HPP
#define BOOST_ALIGN_DETAIL_ASSUME_ALIGNED_CLANG_HPP

#include <stdint.h>

#if defined(__has_builtin) && __has_builtin(__builtin_assume)
#define BOOST_ALIGN_ASSUME_ALIGNED(ptr, alignment) \
__builtin_assume((uintptr_t(ptr) & ((alignment) - 1)) == 0)
#else
#define BOOST_ALIGN_ASSUME_ALIGNED(ptr, alignment)
#endif

#endif
