#pragma once

#if defined(ANDROID)
  #define OMIM_OS_ANDROID
  #define OMIM_OS_NAME "android"

#elif defined(_BADA_SIMULATOR) || defined(_BADA_DEVICE)
  #define OMIM_OS_BADA
  #define OMIM_OS_NAME "bada"

#elif defined(__APPLE__)
  #include <TargetConditionals.h>
  #if (TARGET_OS_IPHONE > 0)
    #define OMIM_OS_IPHONE
    #define OMIM_OS_NAME "ios"
    #if (TARGET_IPHONE_SIMULATOR > 0)
      #define OMIM_OS_IPHONE_SIMULATOR
    #else
      #define OMIM_OS_IPHONE_DEVICE
    #endif
  #else
    #define OMIM_OS_MAC
    #define OMIM_OS_NAME "mac"
  #endif

#elif defined(_WIN32)
  #define OMIM_OS_WINDOWS
  #define OMIM_OS_NAME "win"

  #ifdef __MINGW32__
    #define OMIM_OS_WINDOWS_MINGW
  #else
    #define OMIM_OS_WINDOWS_NATIVE
  #endif
#else
  #define OMIM_OS_LINUX
  #define OMIM_OS_NAME "linux"
#endif
