#pragma once

#ifdef new
#undef new
#endif

#include <unordered_set>
using std::unordered_set;
using std::unordered_multiset;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
