#include "../std/target_os.hpp"

#if defined(OMIM_OS_BADA)
  #include "condition_bada.cpp"
#elif defined(OMIM_OS_WINDOWS_NATIVE)
  #include "condition_windows_native.cpp"
#else
  #include "condition_posix.cpp"
#endif
