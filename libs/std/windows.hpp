#pragma once

#include "target_os.hpp"

#ifdef OMIM_OS_WINDOWS
#include <windows.h>

#undef min
#undef max
#undef FindResource
#undef GetObject

#endif  // OMIM_OS_WINDOWS
