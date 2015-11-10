#pragma once

#ifdef new
#undef new
#endif

#include <mutex>

using std::lock_guard;
using std::mutex;
using std::timed_mutex;
using std::unique_lock;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
