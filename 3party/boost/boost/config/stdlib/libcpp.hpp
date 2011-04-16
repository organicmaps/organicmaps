//  (C) Copyright Christopher Jefferson 2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

//  config for libc++
//  Might need more in here later.

#if !defined(_LIBCPP_VERSION)
#  include <ciso646>
#  if !defined(_LIBCPP_VERSION)
#      error "This is not libc++!"
#  endif
#endif

#define BOOST_STDLIB "libc++ version " BOOST_STRINGIZE(_LIBCPP_VERSION)

#define BOOST_HAS_THREADS

#define BOOST_NO_0X_HDR_CONCEPTS
#define BOOST_NO_0X_HDR_CONTAINER_CONCEPTS
#define BOOST_NO_0X_HDR_ITERATOR_CONCEPTS
#define BOOST_NO_0X_HDR_MEMORY_CONCEPTS

#ifdef _LIBCPP_HAS_NO_VARIADICS
#    define BOOST_NO_0X_HDR_TUPLE
#endif

// libc++ uses a non-standard messages_base
#define BOOST_NO_STD_MESSAGES

//  --- end ---
