#pragma once

#include "base/base.hpp"

#include <cstddef>
#include <type_traits>

// #define ENDIAN_IS_BIG

// @TODO(bykoianko) This method returns false since 05.12.2010. That means only little-endian
// architectures are supported. When it's necessary to support a big-endian system:
// * method IsBigEndianMacroBased() should be implemented based on IsLittleEndian() function
// * method SwapIfBigEndianMacroBased() should be implemented based on IsLittleEndian() function
// * all serialization and deserialization of rs_bit_vector and the other rank-select structures
//   should be implemented taking endianness into account

inline bool IsBigEndianMacroBased()
{
#ifdef ENDIAN_IS_BIG
  return true;
#else
  return false;
#endif
}

template <typename T>
T ReverseByteOrder(T t)
{
  static_assert(std::is_integral<T>::value, "Only integral types are supported.");

  T res;
  char const * a = reinterpret_cast<char const *>(&t);
  char * b = reinterpret_cast<char *>(&res);
  for (size_t i = 0; i < sizeof(T); ++i)
    b[i] = a[sizeof(T) - 1 - i];
  return res;
}

template <typename T>
T SwapIfBigEndianMacroBased(T t)
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
  uint8_t const * b = reinterpret_cast<uint8_t const *>(&word);
  return b[0] != 0x0;
}
