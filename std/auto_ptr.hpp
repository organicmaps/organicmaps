#pragma once

#ifdef new
#undef new
#endif

#include <memory>
using std::auto_ptr;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
