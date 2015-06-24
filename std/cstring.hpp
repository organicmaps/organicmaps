// Use this header for the functions:
// - strlen, strcpy, strcmp
// - memcpy, memcmp, memset

#pragma once

#ifdef new
#undef new
#endif

#include <cstring>

// Use string.h header if cstring is absent for target platform.

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
