#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <boost/shared_ptr.hpp>
using boost::shared_ptr;

using boost::static_pointer_cast;

template <typename T>
inline shared_ptr<T> make_shared_ptr(T * t)
{
  return shared_ptr<T>(t);
}

template <typename T, typename U>
inline shared_ptr<T> make_shared_ptr(T * t, U u)
{
  return shared_ptr<T>(t, u);
}

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
