#pragma once

#if defined(_BADA_SIMULATOR) || defined(_BADA_DEVICE)
  #define OMIM_OS_BADA

#elif defined(__APPLE__)
  #include <TargetConditionals.h>
  #if (TARGET_OS_IPHONE > 0)
    #define OMIM_OS_IPHONE
    #if (TARGET_IPHONE_SIMULATOR > 0)
      #define OMIM_OS_IPHONE_SIMULATOR
    #else
      #define OMIM_OS_IPHONE_DEVICE
    #endif
  #else
    #define OMIM_OS_MAC
  #endif

#elif defined(_WIN32)
  #define OMIM_OS_WINDOWS

  #ifdef __MINGW32__
    #define OMIM_OS_WINDOWS_MINGW
  #else
    #define OMIM_OS_WINDOWS_NATIVE
  #endif
#else
  #define OMIM_OS_LINUX
#endif
