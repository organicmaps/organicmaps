#pragma once

#include "../base/base.hpp"
#include "../std/string.hpp"

#include "../base/start_mem_debug.hpp"

#include <boost/type_traits/is_integral.hpp>

namespace impl {
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
  STATIC_ASSERT(sizeof(*container.begin()) == 1);
  ASSERT ( !container.empty(), ("Dereference of container::end() is illegal") );

  return ToHex(&*container.begin(), container.end() - container.begin());
}

template <typename IntT>
inline string NumToHex(IntT n)
{
  STATIC_ASSERT(boost::is_integral<IntT>::value);

  uint8_t buf[sizeof(n)];

  for (size_t i = 0; i < sizeof(n); ++i)
  {
    buf[i] = (n >> ((sizeof(n) - 1) * 8));
    n <<= 8;
  }

  return ToHex(buf, sizeof(buf));
}

inline string FromHex(void const * ptr, size_t size) {
  string result;
  result.resize(size / 2);
  ::impl::FromHexRaw(ptr, size, &result[0]);
  return result;
}

inline string FromHex(string const & src) {
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

#include "../base/stop_mem_debug.hpp"
