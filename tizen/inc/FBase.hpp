#pragma once
#include "../../std/target_os.hpp"
#ifdef OMIM_OS_TIZEN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
  #include <FBase.h>
#pragma clang diagnostic pop
#endif