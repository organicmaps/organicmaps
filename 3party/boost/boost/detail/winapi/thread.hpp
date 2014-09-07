//  thread.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_THREAD_HPP
#define BOOST_DETAIL_WINAPI_THREAD_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/GetCurrentThread.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost
{
namespace detail
{
namespace winapi
{
#if defined( BOOST_USE_WINDOWS_H )
    using ::GetCurrentThreadId;
    using ::SleepEx;
    using ::Sleep;
    using ::SwitchToThread;
#else
extern "C" {
# ifndef UNDER_CE
    __declspec(dllimport) DWORD_ WINAPI GetCurrentThreadId(void);
    __declspec(dllimport) DWORD_ WINAPI SleepEx(DWORD_, BOOL_);
    __declspec(dllimport) void WINAPI Sleep(DWORD_);
    __declspec(dllimport) BOOL_ WINAPI SwitchToThread(void);
#else
    using ::GetCurrentThreadId;
    using ::SleepEx;
    using ::Sleep;
    using ::SwitchToThread;
#endif
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_THREAD_HPP
