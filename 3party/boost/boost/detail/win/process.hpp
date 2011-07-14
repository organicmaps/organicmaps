//  process.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_PROCESS_HPP
#define BOOST_DETAIL_WIN_PROCESS_HPP

#include <boost/detail/win/basic_types.hpp>
#include <boost/detail/win/GetCurrentProcess.hpp>

namespace boost {
namespace detail {
namespace win32 {
#if defined( BOOST_USE_WINDOWS_H )
    using ::GetCurrentProcessId;
#else
# ifndef UNDER_CE
extern "C" { 
    __declspec(dllimport) unsigned long __stdcall 
        GetCurrentProcessId(void);
}    
# else
    using ::GetCurrentProcessId;
# endif
#endif
}
}
}
#endif // BOOST_DETAIL_WIN_PROCESS_HPP
