#pragma once

// wrapper for correct platform-dependent curl defines

#include "../../../../std/target_os.hpp"

#if defined(OMIM_OS_IPHONE_DEVICE)
  #include "curlbuild-iphone.h"
#elif defined(OMIM_OS_IPHONE_SIMULATOR)
  #include "curlbuild-iphonesim.h"
#elif defined(OMIM_OS_MAC)
  #include "curlbuild-mac.h"
#elif defined(OMIM_OS_WINDOWS)
  #include "curlbuild-generic.h"
#else
  #error Unsupported platform :(
#endif