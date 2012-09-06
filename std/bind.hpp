#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_OS_WINDOWS
  #define BOOST_BIND_ENABLE_STDCALL
#endif

#include <boost/bind.hpp>
using boost::bind;
using boost::ref;
using boost::cref;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
