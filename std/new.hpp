#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <new>

using std::bad_alloc;
using std::bad_array_new_length;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
