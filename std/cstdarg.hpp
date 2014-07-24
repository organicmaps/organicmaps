#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <cstdarg>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
