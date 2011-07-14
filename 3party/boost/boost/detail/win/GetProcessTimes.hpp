//  GetProcessTimes.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_GETPROCESSTIMES_HPP
#define BOOST_DETAIL_WIN_GETPROCESSTIMES_HPP

#include <boost/detail/win/time.hpp>

namespace boost {
namespace detail {
namespace win32 {
#if !defined(UNDER_CE)  // Windows CE does not define GetProcessTimes
#if defined( BOOST_USE_WINDOWS_H )
    using ::GetProcessTimes;
#else
    extern "C" __declspec(dllimport) BOOL_ WINAPI
        GetProcessTimes(
            HANDLE_ hProcess,
            LPFILETIME_ lpCreationTime,
            LPFILETIME_ lpExitTime,
            LPFILETIME_ lpKernelTime,
            LPFILETIME_ lpUserTime
        );
#endif
#endif
}
}
}

#endif // BOOST_DETAIL_WIN_GETPROCESSTIMES_HPP
