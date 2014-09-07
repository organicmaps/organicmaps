//  synchronizaion.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP
#define BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP

#include <boost/detail/winapi/basic_types.hpp>

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
    typedef ::CRITICAL_SECTION CRITICAL_SECTION_;
    typedef ::PAPCFUNC PAPCFUNC_;

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    typedef ::INIT_ONCE INIT_ONCE_;
    typedef ::PINIT_ONCE PINIT_ONCE_;
    typedef ::LPINIT_ONCE LPINIT_ONCE_;
    #define BOOST_DETAIL_WINAPI_INIT_ONCE_STATIC_INIT INIT_ONCE_STATIC_INIT
    typedef ::PINIT_ONCE_FN PINIT_ONCE_FN_;

    typedef ::SRWLOCK SRWLOCK_;
    typedef ::PSRWLOCK PSRWLOCK_;
    #define BOOST_DETAIL_WINAPI_SRWLOCK_INIT SRWLOCK_INIT

    typedef ::CONDITION_VARIABLE CONDITION_VARIABLE_;
    typedef ::PCONDITION_VARIABLE PCONDITION_VARIABLE_;
    #define BOOST_DETAIL_WINAPI_CONDITION_VARIABLE_INIT CONDITION_VARIABLE_INIT
#endif

    using ::InitializeCriticalSection;
#if BOOST_USE_WINAPI_VERSION >= 0x0403
    using ::InitializeCriticalSectionAndSpinCount;
#endif
    using ::EnterCriticalSection;
    using ::TryEnterCriticalSection;
    using ::LeaveCriticalSection;
    using ::DeleteCriticalSection;

# ifdef BOOST_NO_ANSI_APIS
    using ::CreateMutexW;
    using ::OpenMutexW;
    using ::CreateEventW;
    using ::OpenEventW;
    using ::CreateSemaphoreW;
    using ::OpenSemaphoreW;
# else
    using ::CreateMutexA;
    using ::OpenMutexA;
    using ::CreateEventA;
    using ::OpenEventA;
    using ::CreateSemaphoreA;
    using ::OpenSemaphoreA;
# endif
    using ::ReleaseMutex;
    using ::ReleaseSemaphore;
    using ::SetEvent;
    using ::ResetEvent;
    using ::WaitForMultipleObjects;
    using ::WaitForSingleObject;
    using ::QueueUserAPC;

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    using ::InitOnceInitialize;
    using ::InitOnceExecuteOnce;
    using ::InitOnceBeginInitialize;
    using ::InitOnceComplete;

    using ::InitializeSRWLock;
    using ::AcquireSRWLockExclusive;
    using ::TryAcquireSRWLockExclusive;
    using ::ReleaseSRWLockExclusive;
    using ::AcquireSRWLockShared;
    using ::TryAcquireSRWLockShared;
    using ::ReleaseSRWLockShared;

    using ::InitializeConditionVariable;
    using ::WakeConditionVariable;
    using ::WakeAllConditionVariable;
    using ::SleepConditionVariableCS;
    using ::SleepConditionVariableSRW;
#endif

    const DWORD_ infinite       = INFINITE;
    const DWORD_ wait_abandoned = WAIT_ABANDONED;
    const DWORD_ wait_object_0  = WAIT_OBJECT_0;
    const DWORD_ wait_timeout   = WAIT_TIMEOUT;
    const DWORD_ wait_failed    = WAIT_FAILED;

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    const DWORD_ init_once_async = INIT_ONCE_ASYNC;
    const DWORD_ init_once_check_only = INIT_ONCE_CHECK_ONLY;
    const DWORD_ init_once_init_failed = INIT_ONCE_INIT_FAILED;
    const DWORD_ init_once_ctx_reserved_bits = INIT_ONCE_CTX_RESERVED_BITS;

    const ULONG_ condition_variable_lockmode_shared = CONDITION_VARIABLE_LOCKMODE_SHARED;
#endif

#else // defined( BOOST_USE_WINDOWS_H )

extern "C" {

    typedef struct CRITICAL_SECTION_
    {
        struct critical_section_debug * DebugInfo;
        long LockCount;
        long RecursionCount;
        void * OwningThread;
        void * LockSemaphore;
    #if defined(_WIN64)
        unsigned __int64 SpinCount;
    #else
        unsigned long SpinCount;
    #endif
    }
    *PCRITICAL_SECTION_;

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    typedef union INIT_ONCE_
    {
        PVOID_ Ptr;
    }
    *PINIT_ONCE_, *LPINIT_ONCE_;
    #define BOOST_DETAIL_WINAPI_INIT_ONCE_STATIC_INIT {0}
    typedef BOOL_ (WINAPI *PINIT_ONCE_FN_)(PINIT_ONCE_ InitOnce, PVOID_ Parameter, PVOID_ *Context);

    typedef struct SRWLOCK_
    {
        PVOID_ Ptr;
    }
    * PSRWLOCK_;
    #define BOOST_DETAIL_WINAPI_SRWLOCK_INIT {0}

    typedef struct CONDITION_VARIABLE_
    {
        PVOID_ Ptr;
    }
    * PCONDITION_VARIABLE_;
    #define BOOST_DETAIL_WINAPI_CONDITION_VARIABLE_INIT {0}

#endif

    __declspec(dllimport) void WINAPI
        InitializeCriticalSection(PCRITICAL_SECTION_);
#if BOOST_USE_WINAPI_VERSION >= 0x0403
    __declspec(dllimport) BOOL_ WINAPI
        InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION_* lpCS, DWORD_ dwSpinCount);
#endif
    __declspec(dllimport) void WINAPI
        EnterCriticalSection(PCRITICAL_SECTION_);
    __declspec(dllimport) BOOL_ WINAPI
        TryEnterCriticalSection(PCRITICAL_SECTION_);
    __declspec(dllimport) void WINAPI
        LeaveCriticalSection(PCRITICAL_SECTION_);
    __declspec(dllimport) void WINAPI
        DeleteCriticalSection(PCRITICAL_SECTION_);

     struct _SECURITY_ATTRIBUTES;
# ifdef BOOST_NO_ANSI_APIS
    __declspec(dllimport) HANDLE_ WINAPI
        CreateMutexW(_SECURITY_ATTRIBUTES*, BOOL_, LPCWSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenMutexW(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCWSTR_ lpName);
    __declspec(dllimport) HANDLE_ WINAPI
        CreateSemaphoreW(_SECURITY_ATTRIBUTES*, LONG_, LONG_, LPCWSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenSemaphoreW(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCWSTR_ lpName);
    __declspec(dllimport) HANDLE_ WINAPI
        CreateEventW(_SECURITY_ATTRIBUTES*, BOOL_, BOOL_, LPCWSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenEventW(DWORD_, BOOL_, LPCWSTR_);
# else
    __declspec(dllimport) HANDLE_ WINAPI
        CreateMutexA(_SECURITY_ATTRIBUTES*, BOOL_, LPCSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenMutexA(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCSTR_ lpName);
    __declspec(dllimport) HANDLE_ WINAPI
        CreateSemaphoreA(_SECURITY_ATTRIBUTES*, LONG_, LONG_, LPCSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenSemaphoreA(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCSTR_ lpName);
    __declspec(dllimport) HANDLE_ WINAPI
        CreateEventA(_SECURITY_ATTRIBUTES*, BOOL_, BOOL_, LPCSTR_);
    __declspec(dllimport) HANDLE_ WINAPI
        OpenEventA(DWORD_, BOOL_, LPCSTR_);
# endif
    __declspec(dllimport) BOOL_ WINAPI
        ReleaseMutex(HANDLE_);
    __declspec(dllimport) DWORD_ WINAPI
        WaitForSingleObject(HANDLE_, DWORD_);
    __declspec(dllimport) DWORD_ WINAPI
        WaitForMultipleObjects(DWORD_ nCount,
                HANDLE_ const * lpHandles,
                BOOL_ bWaitAll,
                DWORD_ dwMilliseconds);
    __declspec(dllimport) BOOL_ WINAPI
        ReleaseSemaphore(HANDLE_, LONG_, LONG_*);
    __declspec(dllimport) BOOL_ WINAPI
        SetEvent(HANDLE_);
    __declspec(dllimport) BOOL_ WINAPI
        ResetEvent(HANDLE_);

    typedef void (__stdcall *PAPCFUNC_)(ULONG_PTR_);
    __declspec(dllimport) DWORD_ WINAPI
        QueueUserAPC(PAPCFUNC_, HANDLE_, ULONG_PTR_);

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    __declspec(dllimport) void WINAPI InitOnceInitialize(PINIT_ONCE_);
    __declspec(dllimport) BOOL_ WINAPI InitOnceExecuteOnce(PINIT_ONCE_ InitOnce, PINIT_ONCE_FN_ InitFn, PVOID_ Parameter, LPVOID_* Context);
    __declspec(dllimport) BOOL_ WINAPI InitOnceBeginInitialize(LPINIT_ONCE_ lpInitOnce, DWORD_ dwFlags, BOOL_* fPending, LPVOID_* lpContext);
    __declspec(dllimport) BOOL_ WINAPI InitOnceComplete(LPINIT_ONCE_ lpInitOnce, DWORD_ dwFlags, LPVOID_* lpContext);


    __declspec(dllimport) void WINAPI InitializeSRWLock(PSRWLOCK_ SRWLock);
    __declspec(dllimport) void WINAPI AcquireSRWLockExclusive(PSRWLOCK_ SRWLock);
    __declspec(dllimport) BOOLEAN_ WINAPI TryAcquireSRWLockExclusive(PSRWLOCK_ SRWLock);
    __declspec(dllimport) void WINAPI ReleaseSRWLockExclusive(PSRWLOCK_ SRWLock);
    __declspec(dllimport) void WINAPI AcquireSRWLockShared(PSRWLOCK_ SRWLock);
    __declspec(dllimport) BOOLEAN_ WINAPI TryAcquireSRWLockShared(PSRWLOCK_ SRWLock);
    __declspec(dllimport) void WINAPI ReleaseSRWLockShared(PSRWLOCK_ SRWLock);

    __declspec(dllimport) void WINAPI InitializeConditionVariable(PCONDITION_VARIABLE_ ConditionVariable);
    __declspec(dllimport) void WINAPI WakeConditionVariable(PCONDITION_VARIABLE_ ConditionVariable);
    __declspec(dllimport) void WINAPI WakeAllConditionVariable(PCONDITION_VARIABLE_ ConditionVariable);
    __declspec(dllimport) BOOL_ WINAPI SleepConditionVariableCS(PCONDITION_VARIABLE_ ConditionVariable, PCRITICAL_SECTION_ CriticalSection, DWORD_ dwMilliseconds);
    __declspec(dllimport) BOOL_ WINAPI SleepConditionVariableSRW(PCONDITION_VARIABLE_ ConditionVariable, PSRWLOCK_ SRWLock, DWORD_ dwMilliseconds, ULONG_ Flags);
#endif

} // extern "C"

const DWORD_ infinite       = (DWORD_)0xFFFFFFFF;
const DWORD_ wait_abandoned = 0x00000080L;
const DWORD_ wait_object_0  = 0x00000000L;
const DWORD_ wait_timeout   = 0x00000102L;
const DWORD_ wait_failed    = (DWORD_)0xFFFFFFFF;

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
const DWORD_ init_once_async = 0x00000002UL;
const DWORD_ init_once_check_only = 0x00000001UL;
const DWORD_ init_once_init_failed = 0x00000004UL;
const DWORD_ init_once_ctx_reserved_bits = 2;

const ULONG_ condition_variable_lockmode_shared = 0x00000001;
#endif

#endif // defined( BOOST_USE_WINDOWS_H )

const DWORD_ max_non_infinite_wait = (DWORD_)0xFFFFFFFE;

BOOST_FORCEINLINE HANDLE_ create_anonymous_mutex(_SECURITY_ATTRIBUTES* lpAttributes, BOOL_ bInitialOwner)
{
#ifdef BOOST_NO_ANSI_APIS
    return CreateMutexW(lpAttributes, bInitialOwner, 0);
#else
    return CreateMutexA(lpAttributes, bInitialOwner, 0);
#endif
}

BOOST_FORCEINLINE HANDLE_ create_anonymous_semaphore(_SECURITY_ATTRIBUTES* lpAttributes, LONG_ lInitialCount, LONG_ lMaximumCount)
{
#ifdef BOOST_NO_ANSI_APIS
    return CreateSemaphoreW(lpAttributes, lInitialCount, lMaximumCount, 0);
#else
    return CreateSemaphoreA(lpAttributes, lInitialCount, lMaximumCount, 0);
#endif
}

BOOST_FORCEINLINE HANDLE_ create_anonymous_event(_SECURITY_ATTRIBUTES* lpAttributes, BOOL_ bManualReset, BOOL_ bInitialState)
{
#ifdef BOOST_NO_ANSI_APIS
    return CreateEventW(lpAttributes, bManualReset, bInitialState, 0);
#else
    return CreateEventA(lpAttributes, bManualReset, bInitialState, 0);
#endif
}

}
}
}

#endif // BOOST_DETAIL_WINAPI_SYNCHRONIZATION_HPP
