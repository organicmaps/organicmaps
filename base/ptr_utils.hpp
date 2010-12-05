#pragma once

#include "../std/shared_ptr.hpp"

template <typename T>
shared_ptr<T> make_shared_ptr(T * t)
{
  return shared_ptr<T>(t);
}

template <typename T, typename U>
shared_ptr<T> make_shared_ptr(T * t, U u)
{
  return shared_ptr<T>(t, u);
}
