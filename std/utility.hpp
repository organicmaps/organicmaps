#pragma once

#ifdef new
#undef new
#endif

#include <utility>

using std::forward;
using std::make_pair;
using std::move;
using std::pair;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
