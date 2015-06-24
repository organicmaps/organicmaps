#pragma once

#ifdef new
#undef new
#endif

#include <tuple>

using std::tuple;
using std::make_tuple;
//using std::get; // "get" is very common name, use "get" member function

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
