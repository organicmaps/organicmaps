#pragma once

#ifdef new
#undef new
#endif

#include <memory>
using std::unique_ptr;

/// @todo(y): replace this hand-written helper function by
/// std::make_unique when it will be available in C++14
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args &&... args)
{
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
