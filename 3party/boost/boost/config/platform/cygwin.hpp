// (C) Copyright John Maddock 2001 - 2003
// (C) Copyright Bryce Lelbach 2010
//
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org for most recent version.

#define BOOST_PLATFORM          "Cygwin"   // Platform name.
#define BOOST_CYGWIN            __CYGWIN__ // Boost platform ID macros.

#define BOOST_HAS_DIRENT_H
#define BOOST_HAS_LOG1P
#define BOOST_HAS_EXPM1

#define BOOST_NIX            1 
//#define BOOST_GENETIC_NIX    1
//#define BOOST_TRADEMARK_NIX  1
#define BOOST_FUNCTIONAL_NIX 1

// See if we have POSIX threads, otherwise revert to native Win threads.
#define BOOST_HAS_UNISTD_H
#include <unistd.h>

#if defined(_POSIX_THREADS) && (_POSIX_THREADS + 0 >= 0) && \
    !defined(BOOST_HAS_WINTHREADS)
  #define BOOST_HAS_PTHREADS
  #define BOOST_HAS_SCHED_YIELD
  #define BOOST_HAS_GETTIMEOFDAY
  #define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
  #define BOOST_HAS_SIGACTION
#else
  #if !defined(BOOST_HAS_WINTHREADS)
    #define BOOST_HAS_WINTHREADS
  #endif
  #define BOOST_HAS_FTIME
#endif

// Find out if we have a stdint.h, there should be a better way to do this.
#include <sys/types.h>

#ifdef _STDINT_H
  #define BOOST_HAS_STDINT_H
#endif

/// Cygwin has no fenv.h
#define BOOST_NO_FENV_H

#include <boost/config/posix_features.hpp>

// Cygwin lies about XSI conformance, there is no nl_types.h.
#ifdef BOOST_HAS_NL_TYPES_H
  #undef BOOST_HAS_NL_TYPES_H
#endif
 




