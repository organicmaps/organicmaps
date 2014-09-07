//  thread_pool.hpp  --------------------------------------------------------------//

//  Copyright 2013 Andrey Semashev

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_THREAD_POOL_HPP
#define BOOST_DETAIL_WINAPI_THREAD_POOL_HPP

#include <boost/detail/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN2K

namespace boost
{
namespace detail
{
namespace winapi
{
#if defined( BOOST_USE_WINDOWS_H )

typedef ::WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACKFUNC_;
typedef ::WAITORTIMERCALLBACK WAITORTIMERCALLBACK_;

using ::RegisterWaitForSingleObject;
using ::UnregisterWait;
using ::UnregisterWaitEx;

const ULONG_ wt_execute_default = WT_EXECUTEDEFAULT;
const ULONG_ wt_execute_in_io_thread = WT_EXECUTEINIOTHREAD;
const ULONG_ wt_execute_in_ui_thread = WT_EXECUTEINUITHREAD;
const ULONG_ wt_execute_in_wait_thread = WT_EXECUTEINWAITTHREAD;
const ULONG_ wt_execute_only_once = WT_EXECUTEONLYONCE;
const ULONG_ wt_execute_in_timer_thread = WT_EXECUTEINTIMERTHREAD;
const ULONG_ wt_execute_long_function = WT_EXECUTELONGFUNCTION;
const ULONG_ wt_execute_in_persistent_io_thread = WT_EXECUTEINPERSISTENTIOTHREAD;
const ULONG_ wt_execute_in_persistent_thread = WT_EXECUTEINPERSISTENTTHREAD;
const ULONG_ wt_transfer_impersonation = WT_TRANSFER_IMPERSONATION;

inline ULONG_ wt_set_max_threadpool_threads(ULONG_ flags, ULONG_ limit)
{
    return WT_SET_MAX_THREADPOOL_THREADS(flags, limit);
}

#else

extern "C" {

typedef void (NTAPI* WAITORTIMERCALLBACKFUNC_) (PVOID_, BOOLEAN_);
typedef WAITORTIMERCALLBACKFUNC_ WAITORTIMERCALLBACK_;

__declspec(dllimport) BOOL_ WINAPI RegisterWaitForSingleObject
(
    HANDLE_* phNewWaitObject,
    HANDLE_ hObject,
    WAITORTIMERCALLBACK_ Callback,
    PVOID_ Context,
    ULONG_ dwMilliseconds,
    ULONG_ dwFlags
);

__declspec(dllimport) BOOL_ WINAPI UnregisterWait(HANDLE_ WaitHandle);
__declspec(dllimport) BOOL_ WINAPI UnregisterWaitEx(HANDLE_ WaitHandle, HANDLE_ CompletionEvent);

} // extern "C"

const ULONG_ wt_execute_default = 0x00000000;
const ULONG_ wt_execute_in_io_thread = 0x00000001;
const ULONG_ wt_execute_in_ui_thread = 0x00000002;
const ULONG_ wt_execute_in_wait_thread = 0x00000004;
const ULONG_ wt_execute_only_once = 0x00000008;
const ULONG_ wt_execute_in_timer_thread = 0x00000020;
const ULONG_ wt_execute_long_function = 0x00000010;
const ULONG_ wt_execute_in_persistent_io_thread = 0x00000040;
const ULONG_ wt_execute_in_persistent_thread = 0x00000080;
const ULONG_ wt_transfer_impersonation = 0x00000100;

inline ULONG_ wt_set_max_threadpool_threads(ULONG_ flags, ULONG_ limit)
{
    return flags | (limit << 16);
}

#endif
}
}
}

#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN2K

#endif // BOOST_DETAIL_WINAPI_THREAD_POOL_HPP
