//  system.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WIN_SYSTEM_HPP
#define BOOST_DETAIL_WIN_SYSTEM_HPP
#include <boost/config.hpp>
#include <cstdarg>

#include <boost/detail/win/basic_types.hpp>
extern "C" __declspec(dllimport) void __stdcall GetSystemInfo (struct system_info *);

namespace boost {
namespace detail {
namespace win32 {
#if defined( BOOST_USE_WINDOWS_H )
    typedef ::SYSTEM_INFO SYSTEM_INFO_;
#else
extern "C" {
    typedef struct _SYSTEM_INFO {
      union {
        DWORD_  dwOemId;
        struct {
          WORD_ wProcessorArchitecture;
          WORD_ wReserved;
        } dummy;
      } ;
      DWORD_     dwPageSize;
      LPVOID_    lpMinimumApplicationAddress;
      LPVOID_    lpMaximumApplicationAddress;
      DWORD_PTR_ dwActiveProcessorMask;
      DWORD_     dwNumberOfProcessors;
      DWORD_     dwProcessorType;
      DWORD_     dwAllocationGranularity;
      WORD_      wProcessorLevel;
      WORD_      wProcessorRevision;
    } SYSTEM_INFO_;

    __declspec(dllimport) void __stdcall 
        GetSystemInfo (struct system_info *);
}    
#endif
}
}
}
#endif // BOOST_DETAIL_WIN_TIME_HPP
