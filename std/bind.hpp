#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/bind.hpp>
using boost::bind;
using boost::ref;
using boost::cref;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
