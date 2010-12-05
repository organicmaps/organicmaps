#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/noncopyable.hpp>
using boost::noncopyable;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
