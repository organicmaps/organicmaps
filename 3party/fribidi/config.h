#include "std/target_os.hpp"

#if defined(OMIM_OS_MAC)
  #include "config_mac.h"

#elif defined(OMIM_OS_ANDROID)
  #include "config_android.h"

#elif defined(OMIM_OS_IPHONE)
  #include "config_ios.h"

#elif defined(OMIM_OS_WINDOWS)
  #include "config_win32.h"

#elif defined(OMIM_OS_LINUX)
  #include "config_linux.h"

#elif defined(OMIM_OS_TIZEN)
  #include "config_tizen.h"

#else
#error "Add your platform"
#endif
