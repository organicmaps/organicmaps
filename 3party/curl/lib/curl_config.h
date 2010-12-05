#pragma once

// ********************************************************
// Manually created config to support necessary platforms *
// ********************************************************

#if defined(__APPLE__)
  #include <TargetConditionals.h>
  #if (TARGET_OS_IPHONE > 0)
    #if (TARGET_IPHONE_SIMULATOR > 0)
      #include "config-iphonesim.h"
    #else
      #include "config-iphone.h"
    #endif
  #else
    #include "config-mac64.h"
  #endif
#elif defined(WIN32)
  #define WIN32_LEAN_AND_MEAN
  #define WINVER 0x0500
  #define _WIN32_WINNT 0x0500
  #include "config-win32.h"
  // to use sync dns resolve only
  #undef USE_THREADS_WIN32
  // to avoid some mingw warnings
  #ifdef __MINGW32__
    #define HAVE_VARIADIC_MACROS_GCC 1
  #endif
#else
  #error "TODO - unsupported platform"
#endif
