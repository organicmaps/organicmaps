#pragma once

#include "std/algorithm.hpp"

// Calls swap() function using argument dependant lookup.
// Do NOT override this function, but override swap() function instead!
template <typename T> inline void Swap(T & a, T & b)
{
  using std::swap;
  swap(a, b);
}
