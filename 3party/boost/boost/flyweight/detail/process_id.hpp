/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_PROCESS_ID_HPP
#define BOOST_FLYWEIGHT_DETAIL_PROCESS_ID_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS)&&!defined(BOOST_DISABLE_WIN32)

#if defined(BOOST_USE_WINDOWS_H)
#include <windows.h>
#else
namespace boost{
namespace flyweights{
namespace detail{

extern "C" __declspec(dllimport)
unsigned long __stdcall GetCurrentProcessId(void);

} /* namespace flyweights::detail */
} /* namespace flyweights */
} /* namespace boost */
#endif

namespace boost{

namespace flyweights{

namespace detail{

typedef unsigned long process_id_t;

inline process_id_t process_id()
{
  return GetCurrentProcessId();
}

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */

#elif defined(BOOST_HAS_UNISTD_H)

#include <unistd.h>

namespace boost{

namespace flyweights{

namespace detail{

typedef pid_t process_id_t;

inline process_id_t process_id()
{
  return ::getpid();
}

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */

#else
#error Unknown platform
#endif

#endif
