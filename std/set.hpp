#pragma once

#ifdef new
#undef new
#endif

#include <set>
using std::multiset;
using std::set;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
