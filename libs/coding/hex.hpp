#pragma once

#include "base/base.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace impl
{
void ToHexRaw(void const * src, size_t size, void * dst);
void FromHexRaw(void const * src, size_t size, void * dst);
}  // namespace impl

inline std::string ToHex(void const * ptr, size_t size)
{
  std::string result;
  if (size == 0)
    return result;

  result.resize(size * 2);
  ::impl::ToHexRaw(ptr, size, &result[0]);

  return result;
}

template <typename ContainerT>
inline std::string ToHex(ContainerT const & container)
{
  static_assert(sizeof(*container.begin()) == 1, "");

  if (container.empty())
    return {};

  return ToHex(&*container.begin(), container.end() - container.begin());
}

/// Conversion with specializations to avoid warnings
/// @{
template <typename IntT>
inline std::string NumToHex(IntT n)
{
  static_assert(std::is_integral<IntT>::value, "");

  uint8_t buf[sizeof(n)];

  for (size_t i = 0; i < sizeof(n); ++i)
  {
    buf[i] = (n >> ((sizeof(n) - 1) * 8));
    n <<= 8;
  }

  return ToHex(buf, sizeof(buf));
}

template <>
inline std::string NumToHex<int8_t>(int8_t c)
{
  return ToHex(&c, sizeof(c));
}

template <>
inline std::string NumToHex<uint8_t>(uint8_t c)
{
  return ToHex(&c, sizeof(c));
}

template <>
inline std::string NumToHex<char>(char c)
{
  return ToHex(&c, sizeof(c));
}
/// @}

inline std::string FromHex(std::string_view s)
{
  std::string result;
  result.resize(s.size() / 2);
  ::impl::FromHexRaw(s.data(), s.size(), &result[0]);
  return result;
}

inline std::string ByteToQuat(uint8_t n)
{
  std::string result;
  for (size_t i = 0; i < 4; ++i)
  {
    result += char(((n & 0xC0) >> 6) + '0');
    n <<= 2;
  }
  return result;
}

template <typename IntT>
inline std::string NumToQuat(IntT n)
{
  std::string result;
  for (size_t i = 0; i < sizeof(n); ++i)
  {
    uint8_t ub = n >> (sizeof(n) * 8 - 8);
    result += ByteToQuat(ub);
    n <<= 8;
  }
  return result;
}
