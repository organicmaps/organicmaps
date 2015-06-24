#pragma once

#include "base/base.hpp"

#include "std/string.hpp"
#include "std/type_traits.hpp"


namespace impl
{
  void ToHexRaw(void const * src, size_t size, void * dst);
  void FromHexRaw(void const * src, size_t size, void * dst);
}

inline string ToHex(const void * ptr, size_t size)
{
  string result;
  if (size == 0) return result;

  result.resize(size * 2);
  ::impl::ToHexRaw(ptr, size, &result[0]);

  return result;
}

template <typename ContainerT>
inline string ToHex(ContainerT const & container)
{
  static_assert(sizeof(*container.begin()) == 1, "");

  if (container.empty())
    return string();

  return ToHex(&*container.begin(), container.end() - container.begin());
}

/// Conversion with specializations to avoid warnings
/// @{
template <typename IntT>
inline string NumToHex(IntT n)
{
  static_assert(is_integral<IntT>::value, "");

  uint8_t buf[sizeof(n)];

  for (size_t i = 0; i < sizeof(n); ++i)
  {
    buf[i] = (n >> ((sizeof(n) - 1) * 8));
    n <<= 8;
  }

  return ToHex(buf, sizeof(buf));
}

template <>
inline string NumToHex<int8_t>(int8_t c)
{
  return ToHex(&c, sizeof(c));
}

template <>
inline string NumToHex<uint8_t>(uint8_t c)
{
  return ToHex(&c, sizeof(c));
}

template <>
inline string NumToHex<char>(char c)
{
  return ToHex(&c, sizeof(c));
}
/// @}

inline string FromHex(void const * ptr, size_t size)
{
  string result;
  result.resize(size / 2);
  ::impl::FromHexRaw(ptr, size, &result[0]);
  return result;
}

inline string FromHex(string const & src)
{
  return FromHex(src.c_str(), src.size());
}

inline string ByteToQuat(uint8_t n)
{
  string result;
  for (size_t i = 0; i < 4; ++i)
  {
    result += char(((n & 0xC0) >> 6) + '0');
    n <<= 2;
  }
  return result;
}

template <typename IntT>
inline string NumToQuat(IntT n)
{
  string result;
  for (size_t i = 0; i < sizeof(n); ++i)
  {
    uint8_t ub = n >> (sizeof(n) * 8 - 8);
    result += ByteToQuat(ub);
    n <<= 8;
  }
  return result;
}
