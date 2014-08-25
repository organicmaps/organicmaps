#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#if (__cplusplus > 199711L) || defined(__GXX_EXPERIMENTAL_CXX0X__)

  #include <functional>
  using std::function;

#else

  #include <boost/function.hpp>
  using boost::function;

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
