#pragma once

#include "std/limits.hpp"

namespace bits
{
  template <class T>
  struct bits_of
  {
    enum
    {
      value = sizeof(T) * CHAR_BIT
    };
  };

  template <class T>
  T ror(T val, unsigned int n)
  {
      enum
      {
        bits = bits_of<T>::value
      };
      n = n % bits;
      return (val >> n) | (val << (bits - n));
  }
}
