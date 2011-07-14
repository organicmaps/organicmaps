//  synchronizaion.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_SYNCHRONIZATION_HPP
#define BOOST_DETAIL_WIN_SYNCHRONIZATION_HPP

#include <boost/detail/win/basic_types.hpp>


namespace boost
{
namespace detail
{
namespace win32
{
#if defined( BOOST_USE_WINDOWS_H )
    typedef ::CRITICAL_SECTION CRITICAL_SECTION_;
    typedef ::PAPCFUNC PAPCFUNC_;

    using ::InitializeCriticalSection;
    using ::EnterCriticalSection;
    using ::TryEnterCriticalSection;
    using ::LeaveCriticalSection;
    using ::DeleteCriticalSection;
    
# ifdef BOOST_NO_ANSI_APIS
    using ::CreateMutexW;
    using ::CreateEventW;
    using ::OpenEventW;
    using ::CreateSemaphoreW;
# else
    using ::CreateMutexA;
    using ::CreateEventA;
    using ::OpenEventA;
    using ::CreateSemaphoreA;
# endif
    using ::ReleaseMutex;
    using ::ReleaseSemaphore;
    using ::SetEvent;
    using ::ResetEvent;
    using ::WaitForMultipleObjects;
    using ::WaitForSingleObject;
            using ::QueueUserAPC;
#else
extern "C" {
    struct CRITICAL_SECTION_
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
    };

     __declspec(dllimport) void __stdcall 
        InitializeCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall 
        EnterCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) bool __stdcall 
        TryEnterCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall 
        LeaveCriticalSection(CRITICAL_SECTION_ *);
    __declspec(dllimport) void __stdcall 
        DeleteCriticalSection(CRITICAL_SECTION_ *);
    
    struct _SECURITY_ATTRIBUTES;
# ifdef BOOST_NO_ANSI_APIS
    __declspec(dllimport) void* __stdcall 
        CreateMutexW(_SECURITY_ATTRIBUTES*,int,wchar_t const*);
    __declspec(dllimport) void* __stdcall 
        CreateSemaphoreW(_SECURITY_ATTRIBUTES*,long,long,wchar_t const*);
    __declspec(dllimport) void* __stdcall 
        CreateEventW(_SECURITY_ATTRIBUTES*,int,int,wchar_t const*);
    __declspec(dllimport) void* __stdcall 
        OpenEventW(unsigned long,int,wchar_t const*);
# else
    __declspec(dllimport) void* __stdcall 
        CreateMutexA(_SECURITY_ATTRIBUTES*,int,char const*);
    __declspec(dllimport) void* __stdcall 
        CreateSemaphoreA(_SECURITY_ATTRIBUTES*,long,long,char const*);
    __declspec(dllimport) void* __stdcall 
        CreateEventA(_SECURITY_ATTRIBUTES*,int,int,char const*);
    __declspec(dllimport) void* __stdcall 
        OpenEventA(unsigned long,int,char const*);
# endif
    __declspec(dllimport) int __stdcall 
        ReleaseMutex(void*);
    __declspec(dllimport) unsigned long __stdcall 
        WaitForSingleObject(void*,unsigned long);
    __declspec(dllimport) unsigned long __stdcall 
        WaitForMultipleObjects(unsigned long nCount,
                void* const * lpHandles,
                int bWaitAll,
                unsigned long dwMilliseconds);
    __declspec(dllimport) int __stdcall 
        ReleaseSemaphore(void*,long,long*);
    typedef void (__stdcall *PAPCFUNC8)(ulong_ptr);
    __declspec(dllimport) unsigned long __stdcall 
        QueueUserAPC(PAPCFUNC8,void*,ulong_ptr);
# ifndef UNDER_CE
    __declspec(dllimport) int __stdcall 
        SetEvent(void*);
    __declspec(dllimport) int __stdcall 
        ResetEvent(void*);
# else
    using ::SetEvent;
    using ::ResetEvent;
# endif
}    
#endif
}
}
}

#endif // BOOST_DETAIL_WIN_SYNCHRONIZATION_HPP
