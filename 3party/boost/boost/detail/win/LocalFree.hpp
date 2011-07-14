//  LocalFree.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_LOCALFREE_HPP
#define BOOST_DETAIL_WIN_LOCALFREE_HPP

#include <boost/detail/win/basic_types.hpp>

namespace boost {
namespace detail {
namespace win32 {
#if defined( BOOST_USE_WINDOWS_H )
    typedef HANDLE_ HLOCAL_;

    using ::LocalFree;
#else
    extern "C" typedef HANDLE_ HLOCAL_;
    extern "C" __declspec(dllimport) HLOCAL_ WINAPI 
        LocalFree(HLOCAL_ hMem);
#endif
}
}
}
#endif // BOOST_DETAIL_WIN_LOCALFREE_HPP
