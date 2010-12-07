#pragma once

// we support only Vista and above at the moment (due to conditional variables)
#ifdef _WIN32_WINNT
  #undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#ifdef WINVER
  #undef WINVER
#endif
#define WINVER 0x0600

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN32_IE
  #undef _WIN32_IE
#endif
#define _WIN32_IE 0x0600

#include <windows.h>

#undef min
#undef max
