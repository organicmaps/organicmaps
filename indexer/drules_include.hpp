#pragma once

#include "../std/target_os.hpp"

#ifdef OMIM_OS_DESKTOP
  #include "drules_struct.pb.h"
#else
  #include "drules_struct_lite.pb.h"
#endif
