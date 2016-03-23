#pragma once

#ifdef new
#undef new
#endif

#include <memory>
using std::shared_ptr;
using std::make_shared;
using std::enable_shared_from_this;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
