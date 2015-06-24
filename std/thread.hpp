#pragma once

#ifdef new
#undef new
#endif

#include <thread>

namespace this_thread = std::this_thread;

using std::thread;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
