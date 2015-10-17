//  crypt.hpp  --------------------------------------------------------------//

//  Copyright 2014 Antony Polukhin

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_CRYPT_HPP
#define BOOST_DETAIL_WINAPI_CRYPT_HPP

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
    typedef HCRYPTPROV HCRYPTPROV_;

    using ::CryptEnumProvidersA;
    using ::CryptAcquireContextA;
    using ::CryptGenRandom;
    using ::CryptReleaseContext;

    const DWORD_ PROV_RSA_FULL_         = PROV_RSA_FULL;

    const DWORD_ CRYPT_VERIFYCONTEXT_   = CRYPT_VERIFYCONTEXT;
    const DWORD_ CRYPT_NEWKEYSET_       = CRYPT_NEWKEYSET;
    const DWORD_ CRYPT_DELETEKEYSET_    = CRYPT_DELETEKEYSET;
    const DWORD_ CRYPT_MACHINE_KEYSET_  = CRYPT_MACHINE_KEYSET;
    const DWORD_ CRYPT_SILENT_          = CRYPT_SILENT;
#else
extern "C" {
    typedef ULONG_PTR_ HCRYPTPROV_;

    __declspec(dllimport) BOOL_ __stdcall
        CryptEnumProvidersA(
            DWORD_ dwIndex,
            DWORD_ *pdwReserved,
            DWORD_ dwFlags,
            DWORD_ *pdwProvType,
            LPSTR_ szProvName,
            DWORD_ *pcbProvName
    );

    __declspec(dllimport) BOOL_ __stdcall
        CryptAcquireContextA(
            HCRYPTPROV_ *phProv,
            LPCSTR_ pszContainer,
            LPCSTR_ pszProvider,
            DWORD_ dwProvType,
            DWORD_ dwFlags
    );

    __declspec(dllimport) BOOL_ __stdcall
        CryptGenRandom(
            HCRYPTPROV_ hProv,
            DWORD_ dwLen,
            BYTE_ *pbBuffer
    );

    __declspec(dllimport) BOOL_ __stdcall
        CryptReleaseContext(
            HCRYPTPROV_ hProv,
            DWORD_ dwFlags
    );

    const DWORD_ PROV_RSA_FULL_         = 1;

    const DWORD_ CRYPT_VERIFYCONTEXT_   = 0xF0000000;
    const DWORD_ CRYPT_NEWKEYSET_       = 8;
    const DWORD_ CRYPT_DELETEKEYSET_    = 16;
    const DWORD_ CRYPT_MACHINE_KEYSET_  = 32;
    const DWORD_ CRYPT_SILENT_          = 64;
}
#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_CRYPT_HPP
