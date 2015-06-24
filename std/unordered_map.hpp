#pragma once

#ifdef new
#undef new
#endif

#include <unordered_map>
using std::unordered_map;
using std::unordered_multimap;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
