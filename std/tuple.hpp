#pragma once

#ifdef new
#undef new
#endif

#include <tuple>

using std::tuple;
using std::make_tuple;
using std::get;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
