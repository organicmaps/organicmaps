#pragma once

#include "target_os.hpp"

#ifdef OMIM_OS_WINDOWS
// These defines are moved to common.pri because
// they should be equal for all libraries, even for 3party ones

//#ifdef _WIN32_WINNT
//  #undef _WIN32_WINNT
//#endif
//#define _WIN32_WINNT 0x0501

//#ifdef WINVER
//  #undef WINVER
//#endif
//#define WINVER 0x0501

//#ifndef WIN32_LEAN_AND_MEAN
//  #define WIN32_LEAN_AND_MEAN 1
//#endif

//#ifdef _WIN32_IE
//  #undef _WIN32_IE
//#endif
//#define _WIN32_IE 0x0501

//#ifdef NTDDI_VERSION
//  #undef NTDDI_VERSION
//#endif
//#define NTDDI_VERSION 0x05010000

#include <windows.h>

#undef min
#undef max

#endif // OMIM_OS_WINDOWS
