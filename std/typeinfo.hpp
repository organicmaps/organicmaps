#pragma once

#ifdef new
#undef new
#endif

#include <typeinfo>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
