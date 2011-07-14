//  GetThreadTimes.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_GETTHREADTIMES_HPP
#define BOOST_DETAIL_WIN_GETTHREADTIMES_HPP

#include <boost/detail/win/time.hpp>

namespace boost {
namespace detail {
namespace win32 {
#if defined( BOOST_USE_WINDOWS_H )
    using ::GetThreadTimes;
#else
    extern "C" __declspec(dllimport) BOOL_ WINAPI
        GetThreadTimes(
            HANDLE_ hThread,
            LPFILETIME_ lpCreationTime,
            LPFILETIME_ lpExitTime,
            LPFILETIME_ lpKernelTime,
            LPFILETIME_ lpUserTime
        );
#endif
}
}
}

#endif // BOOST_DETAIL_WIN_GETTHREADTIMES_HPP
