#pragma once

#ifdef new
#undef new
#endif

#include <functional>
using std::equal_to;
using std::function;
using std::greater;
using std::hash;
using std::less;
using std::less_equal;
using std::mem_fn;
using std::ref;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
