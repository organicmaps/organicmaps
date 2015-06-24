#pragma once

#ifdef new
#undef new
#endif

#include <list>
using std::list;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
