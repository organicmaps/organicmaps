#pragma once

#ifdef new
#undef new
#endif

#include <boost/iterator/transform_iterator.hpp>
using boost::make_transform_iterator;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
