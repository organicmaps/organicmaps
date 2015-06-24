#pragma once

#ifdef new
#undef new
#endif

// TODO: Mirgate to C++11 technics to disable copying.
#include <boost/noncopyable.hpp>
using boost::noncopyable;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
