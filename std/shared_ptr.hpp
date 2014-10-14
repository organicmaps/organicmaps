#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#ifdef CPP11_IS_SUPPORTED

#include <memory>
using std::shared_ptr;
using std::make_shared;

#else

#include <boost/shared_ptr.hpp>
using boost::shared_ptr;

using boost::static_pointer_cast;

#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
