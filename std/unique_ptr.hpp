#pragma once

#ifdef new
#undef new
#endif

#include <memory>
using std::unique_ptr;
using std::make_unique;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
