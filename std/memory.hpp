#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <memory>

using std::make_shared;
using std::shared_ptr;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
