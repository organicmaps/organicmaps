#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#if (__cplusplus > 199711L) || defined(__GXX_EXPERIMENTAL_CXX0X__)

#include <functional>
using std::bind;
using std::ref;
using std::cref;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

#else

#ifdef OMIM_OS_WINDOWS
  #define BOOST_BIND_ENABLE_STDCALL
#endif

#include <boost/bind.hpp>
using boost::bind;
using boost::ref;
using boost::cref;

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
