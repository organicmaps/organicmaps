//  GetCurrentThread.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_GETCURRENTTHREAD_HPP
#define BOOST_DETAIL_WIN_GETCURRENTTHREAD_HPP

#include <boost/detail/win/basic_types.hpp>

namespace boost {
namespace detail {
namespace win32 {
#if defined( UNDER_CE )
// Windows CE define GetCurrentThread as an inline function in kfuncs.h
inline HANDLE_ GetCurrentThread() 
{
  return ::GetCurrentThread();
}
#else
#if defined( BOOST_USE_WINDOWS_H )
    using ::GetCurrentThread;
#else
    extern "C" __declspec(dllimport) HANDLE_ WINAPI GetCurrentThread();
#endif
#endif
}
}
}

#endif // BOOST_DETAIL_WIN_TIME_HPP
