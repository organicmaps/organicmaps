#pragma once

#ifdef new
#undef new
#endif

#include <stack>
using std::stack;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
