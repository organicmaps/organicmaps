//  directory_management.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_DIRECTORY_MANAGEMENT_HPP
#define BOOST_DETAIL_WIN_DIRECTORY_MANAGEMENT_HPP

#include <boost/detail/win/basic_types.hpp>
#include <boost/detail/win/security.hpp>

namespace boost
{
namespace detail
{
namespace win32
{
#if defined( BOOST_USE_WINDOWS_H )
    using ::CreateDirectory;
    using ::CreateDirectoryA;
    using ::GetTempPathA;
    using ::RemoveDirectoryA;
#else
extern "C" { 
    __declspec(dllimport) int __stdcall 
        CreateDirectory(LPCTSTR_, LPSECURITY_ATTRIBUTES_*);
    __declspec(dllimport) int __stdcall 
        CreateDirectoryA(LPCTSTR_, interprocess_security_attributes*);
    __declspec(dllimport) int __stdcall 
        GetTempPathA(unsigned long length, char *buffer);
    __declspec(dllimport) int __stdcall 
        RemoveDirectoryA(LPCTSTR_);
    
}    
#endif
}
}
}

#endif // BOOST_DETAIL_WIN_THREAD_HPP
