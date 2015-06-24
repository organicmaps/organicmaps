#pragma once

#ifdef new
#undef new
#endif

#include <map>
using std::map;
using std::multimap;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
