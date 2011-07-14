//  boost/chrono/config.hpp  -------------------------------------------------//

//  Copyright Beman Dawes 2003, 2006, 2008
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/chrono for documentation.

#ifndef BOOST_CHRONO_CONFIG_HPP
#define BOOST_CHRONO_CONFIG_HPP

#include <boost/config.hpp>

#if defined(BOOST_CHRONO_SOURCE) && !defined(BOOST_USE_WINDOWS_H)
#define BOOST_USE_WINDOWS_H
#endif

//  BOOST_CHRONO_POSIX_API, BOOST_CHRONO_MAC_API, or BOOST_CHRONO_WINDOWS_API
//  can be defined by the user to specify which API should be used

#if defined(BOOST_CHRONO_WINDOWS_API)
# warning Boost.Chrono will use the Windows API
#elif defined(BOOST_CHRONO_MAC_API)
# warning Boost.Chrono will use the Mac API
#elif defined(BOOST_CHRONO_POSIX_API)
# warning Boost.Chrono will use the POSIX API
#endif

# if defined( BOOST_CHRONO_WINDOWS_API ) && defined( BOOST_CHRONO_POSIX_API )
#   error both BOOST_CHRONO_WINDOWS_API and BOOST_CHRONO_POSIX_API are defined
# elif defined( BOOST_CHRONO_WINDOWS_API ) && defined( BOOST_CHRONO_MAC_API )
#   error both BOOST_CHRONO_WINDOWS_API and BOOST_CHRONO_MAC_API are defined
# elif defined( BOOST_CHRONO_MAC_API ) && defined( BOOST_CHRONO_POSIX_API )
#   error both BOOST_CHRONO_MAC_API and BOOST_CHRONO_POSIX_API are defined
# elif !defined( BOOST_CHRONO_WINDOWS_API ) && !defined( BOOST_CHRONO_MAC_API ) && !defined( BOOST_CHRONO_POSIX_API )
#   if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))
#     define BOOST_CHRONO_WINDOWS_API
#     define BOOST_CHRONO_HAS_CLOCK_STEADY
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_CHRONO_MAC_API
#     define BOOST_CHRONO_HAS_CLOCK_STEADY
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   else
#     define BOOST_CHRONO_POSIX_API
#   endif
# endif

# if defined( BOOST_CHRONO_POSIX_API )
#   include <time.h>  //to check for CLOCK_REALTIME and CLOCK_MONOTONIC and _POSIX_THREAD_CPUTIME
#   if defined(CLOCK_REALTIME)
#     if defined(CLOCK_MONOTONIC)
#        define BOOST_CHRONO_HAS_CLOCK_STEADY
#     endif
#   else
#     error <time.h> does not supply CLOCK_REALTIME
#   endif
#   if defined(_POSIX_THREAD_CPUTIME) && !defined(BOOST_DISABLE_THREADS)
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   endif
#   if defined(CLOCK_THREAD_CPUTIME_ID) && !defined(BOOST_DISABLE_THREADS)
#     define BOOST_CHRONO_HAS_THREAD_CLOCK
#     define BOOST_CHRONO_THREAD_CLOCK_IS_STEADY true
#   endif
# endif

#if defined(BOOST_CHRONO_THREAD_DISABLED) && defined(BOOST_CHRONO_HAS_THREAD_CLOCK)
#undef BOOST_CHRONO_HAS_THREAD_CLOCK
#undef BOOST_CHRONO_THREAD_CLOCK_IS_STEADY
#endif

// unicode support  ------------------------------//

#if defined(BOOST_NO_UNICODE_LITERALS) || defined(BOOST_NO_CHAR16_T) || defined(BOOST_NO_CHAR32_T)
//~ #define BOOST_CHRONO_HAS_UNICODE_SUPPORT
#else
#define BOOST_CHRONO_HAS_UNICODE_SUPPORT 1
#endif

//  define constexpr related macros  ------------------------------//

#if defined(BOOST_NO_CONSTEXPR)
#define BOOST_CHRONO_CONSTEXPR
#define BOOST_CHRONO_CONSTEXPR_OR_CONST const
#define BOOST_CHRONO_CONST_REF const&
#else
#define BOOST_CHRONO_CONSTEXPR constexpr
#define BOOST_CHRONO_CONSTEXPR_OR_CONST constexpr
#define BOOST_CHRONO_CONST_REF
#endif

#define BOOST_CHRONO_STATIC_CONSTEXPR  static BOOST_CHRONO_CONSTEXPR_OR_CONST


#ifdef BOOST_CHRONO_HEADER_ONLY
#define BOOST_CHRONO_INLINE inline
#define BOOST_CHRONO_STATIC
#define BOOST_CHRONO_DECL

#else
#define BOOST_CHRONO_INLINE
#define BOOST_CHRONO_STATIC static

//  enable dynamic linking on Windows  ---------------------------------------//

#ifdef BOOST_HAS_DECLSPEC // defined by boost.config
// we need to import/export our code only if the user has specifically
// asked for it by defining either BOOST_ALL_DYN_LINK if they want all boost
// libraries to be dynamically linked, or BOOST_CHRONO_DYN_LINK
// if they want just this one to be dynamically liked:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CHRONO_DYN_LINK)
// export if this is our own source, otherwise import:
#ifdef BOOST_CHRONO_SOURCE
# define BOOST_CHRONO_DECL __declspec(dllexport)
#else
# define BOOST_CHRONO_DECL __declspec(dllimport)
#endif  // BOOST_CHRONO_SOURCE
#endif  // DYN_LINK
#endif  // BOOST_HAS_DECLSPEC
//
// if BOOST_CHRONO_DECL isn't defined yet define it now:
#ifndef BOOST_CHRONO_DECL
#define BOOST_CHRONO_DECL
#endif


//  enable automatic library variant selection  ------------------------------//

#if !defined(BOOST_CHRONO_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_CHRONO_NO_LIB)
//
// Set the name of our library; this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_chrono
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_CHRONO_DYN_LINK)
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled
#endif // BOOST_CHRONO_HEADER_ONLY
#endif // BOOST_CHRONO_CONFIG_HPP

