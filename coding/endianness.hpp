#pragma once

#include "base/base.hpp"

#include "std/type_traits.hpp"

#include <cstddef>

// #define ENDIAN_IS_BIG

// @TODO(bykoianko) This method returns false since 05.12.2010. That means only little endian
// architecture is supported. Now checks are added to generator and to app that only
// little endian architecture is supported. All the usage of IsBigEndian(), ReverseByteOrder()
// and SwapIfBigEndian() should be removed.
inline bool IsBigEndian()
{
#ifdef ENDIAN_IS_BIG
  return true;
#else
  return false;
#endif
}

template <typename T> T ReverseByteOrder(T t)
{
  static_assert(is_integral<T>::value, "Only integral types are supported.");

  T res;
  char const * a = reinterpret_cast<char const *>(&t);
  char * b = reinterpret_cast<char *>(&res);
  for (size_t i = 0; i < sizeof(T); ++i)
    b[i] = a[sizeof(T) - 1 - i];
  return res;
}

template <typename T> inline T SwapIfBigEndian(T t)
{
#ifdef ENDIAN_IS_BIG
  return ReverseByteOrder(t);
#else
  return t;
#endif
}

inline bool IsLittleEndian()
{
  uint16_t const word = 0x0001;
  char const * const b = reinterpret_cast<char const * const>(&word);
  return b[0] == 0x1;
}
