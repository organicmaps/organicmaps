//  dll.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_DLL_HPP
#define BOOST_DETAIL_WINAPI_DLL_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/security.hpp>

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
    typedef ::FARPROC FARPROC_;
    typedef ::NEARPROC NEARPROC_;
    typedef ::PROC PROC_;

# ifdef BOOST_NO_ANSI_APIS
    using ::LoadLibraryW;
    using ::GetModuleHandleW;
# else
    using ::LoadLibraryA;
    using ::GetModuleHandleA;
# endif
    using ::FreeLibrary;
    using ::GetProcAddress;
#else
extern "C" {
# ifdef _WIN64
    typedef INT_PTR_ (WINAPI *FARPROC_)();
    typedef INT_PTR_ (WINAPI *NEARPROC_)();
    typedef INT_PTR_ (WINAPI *PROC_)();
# else
    typedef int (WINAPI *FARPROC_)();
    typedef int (WINAPI *NEARPROC_)();
    typedef int (WINAPI *PROC_)();
# endif // _WIN64

# ifdef BOOST_NO_ANSI_APIS
    __declspec(dllimport) HMODULE_ WINAPI
        LoadLibraryW(
            LPCWSTR_ lpFileName
    );
    __declspec(dllimport) HMODULE_ WINAPI
        GetModuleHandleW(
            LPCWSTR_ lpFileName
    );
# else
    __declspec(dllimport) HMODULE_ WINAPI
        LoadLibraryA(
            LPCSTR_ lpFileName
    );
    __declspec(dllimport) HMODULE_ WINAPI
        GetModuleHandleA(
            LPCSTR_ lpFileName
    );
# endif

    __declspec(dllimport) BOOL_ WINAPI
        FreeLibrary(
            HMODULE_ hModule
    );
    __declspec(dllimport) FARPROC_ WINAPI
        GetProcAddress(
            HMODULE_ hModule,
            LPCSTR_ lpProcName
    );
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_DLL_HPP
