#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#ifdef CPP11_IS_SUPPORTED

  #include <functional>
  using std::function;
  using std::greater;

#else

  #include <boost/function.hpp>
  using boost::function;

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
