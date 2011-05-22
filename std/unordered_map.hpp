#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
  // to avoid compilation errors on VS2010
  #define BOOST_NO_0X_HDR_TYPEINDEX
#endif

#include <boost/unordered_map.hpp>
using boost::unordered_map;
using boost::unordered_multimap;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
