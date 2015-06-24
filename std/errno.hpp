#pragma once

#ifdef new
#undef new
#endif

#include <cerrno>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
