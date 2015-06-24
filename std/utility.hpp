#pragma once

#ifdef new
#undef new
#endif

#include <utility>
using std::pair;
using std::make_pair;
using std::move;
using std::forward;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
