#pragma once

#ifdef new
#undef new
#endif

#include <boost/static_assert.hpp>
#define STATIC_ASSERT BOOST_STATIC_ASSERT

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
