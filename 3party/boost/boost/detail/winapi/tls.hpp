//  tls.hpp  --------------------------------------------------------------//

//  Copyright 2013 Andrey Semashev

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_TLS_HPP
#define BOOST_DETAIL_WINAPI_TLS_HPP

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

using ::TlsAlloc;
using ::TlsGetValue;
using ::TlsSetValue;
using ::TlsFree;

const DWORD_ tls_out_of_indexes = TLS_OUT_OF_INDEXES;

#else

extern "C" {
__declspec(dllimport) DWORD_ WINAPI TlsAlloc(void);
__declspec(dllimport) LPVOID_ WINAPI TlsGetValue(DWORD_ dwTlsIndex);
__declspec(dllimport) BOOL_ WINAPI TlsSetValue(DWORD_ dwTlsIndex, LPVOID_ lpTlsValue);
__declspec(dllimport) BOOL_ WINAPI TlsFree(DWORD_ dwTlsIndex);
}

const DWORD_ tls_out_of_indexes = 0xFFFFFFFF;

#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_TLS_HPP
