#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <stdio.h>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
