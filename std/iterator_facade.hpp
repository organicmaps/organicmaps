#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/iterator/iterator_facade.hpp>
using boost::iterator_facade;
using boost::random_access_traversal_tag;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
