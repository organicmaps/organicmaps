#pragma once
#include "write_to_sink.hpp"

#include "../base/assert.hpp"
#include "../base/base.hpp"

#include "../std/type_traits.hpp"


/// This function writes, using optimal bytes count.
/// Pass any integral type and it will write platform-independent.
template <typename T, typename TSink> void WriteVarUint(TSink & dst, T value)
{
  STATIC_ASSERT(is_unsigned<T>::value);
  while (value > 127)
  {
    WriteToSink(dst, static_cast<uint8_t>((value & 127) | 128));
    value >>= 7;
  }
  WriteToSink(dst, static_cast<uint8_t>(value));
}

namespace impl
{

template <typename TSource> uint32_t ReadVarUint(TSource & src, uint32_t const *)
{
  uint32_t res = 0;

  {
    uint8_t next0;
    src.Read(&next0, 1);
    res |= (static_cast<uint32_t>(next0) & 127);
    if (!(next0 & 128)) return res;
  }
  {
    uint8_t next1;
    src.Read(&next1, 1);
    res |= (static_cast<uint32_t>(next1) & 127) << 7;
    if (!(next1 & 128)) return res;
  }
  {
    uint8_t next2;
    src.Read(&next2, 1);
    res |= (static_cast<uint32_t>(next2) & 127) << 14;
    if (!(next2 & 128)) return res;
  }
  {
    uint8_t next3;
    src.Read(&next3, 1);
    res |= (static_cast<uint32_t>(next3) & 127) << 21;
    if (!(next3 & 128)) return res;
  }
  {
    uint8_t next4;
    src.Read(&next4, 1);
    res |= (static_cast<uint32_t>(next4) & 127) << 28;
    ASSERT(!(next4 & 128), (next4));
    ASSERT_LESS(next4, 1 << (32 - 28), ());
    return res;
  }
}

template <typename TSource> uint64_t ReadVarUint(TSource & src, uint64_t const *)
{
  uint32_t res0 = 0;
  {
    uint8_t next0;
    src.Read(&next0, 1);
    res0 |= (static_cast<uint32_t>(next0) & 127);
    if (!(next0 & 128)) return res0;
  }
  {
    uint8_t next1;
    src.Read(&next1, 1);
    res0 |= (static_cast<uint32_t>(next1) & 127) << 7;
    if (!(next1 & 128)) return res0;
  }
  {
    uint8_t next2;
    src.Read(&next2, 1);
    res0 |= (static_cast<uint32_t>(next2) & 127) << 14;
    if (!(next2 & 128)) return res0;
  }
  {
    uint8_t next3;
    src.Read(&next3, 1);
    res0 |= (static_cast<uint32_t>(next3) & 127) << 21;
    if (!(next3 & 128)) return res0;
  }

  uint32_t res1 = 0;
  {
    uint8_t next4;
    src.Read(&next4, 1);
    res1 |= (static_cast<uint32_t>(next4) & 127);
    if (!(next4 & 128)) return (static_cast<uint64_t>(res1) << 28) + res0;
  }
  {
    uint8_t next5;
    src.Read(&next5, 1);
    res1 |= (static_cast<uint32_t>(next5) & 127) << 7;
    if (!(next5 & 128)) return (static_cast<uint64_t>(res1) << 28) + res0;
  }
  {
    uint8_t next6;
    src.Read(&next6, 1);
    res1 |= (static_cast<uint32_t>(next6) & 127) << 14;
    if (!(next6 & 128)) return (static_cast<uint64_t>(res1) << 28) + res0;
  }
  {
    uint8_t next7;
    src.Read(&next7, 1);
    res1 |= (static_cast<uint32_t>(next7) & 127) << 21;
    if (!(next7 & 128)) return (static_cast<uint64_t>(res1) << 28) + res0;
  }

  uint32_t res2 = 0;

  {
    uint8_t next8;
    src.Read(&next8, 1);
    res2 |= (static_cast<uint32_t>(next8) & 127);
    if (!(next8 & 128))
      return (static_cast<uint64_t>(res2) << 56) + (static_cast<uint64_t>(res1) << 28) + res0;
  }
  {
    uint8_t next9;
    src.Read(&next9, 1);
    res2 |= (static_cast<uint32_t>(next9) & 127) << 7;
    ASSERT(!(next9 & 128), (next9));
    ASSERT_LESS_OR_EQUAL(next9, 1 << (64 - 63), ());
    return (static_cast<uint64_t>(res2) << 56) + (static_cast<uint64_t>(res1) << 28) + res0;
  }
}

}

template <typename T, typename TSource> T ReadVarUint(TSource & src)
{
  STATIC_ASSERT((is_same<T, uint32_t>::value || is_same<T, uint64_t>::value));
  return ::impl::ReadVarUint(src, static_cast<T const *>(NULL));

  /* Generic code commented out.
  STATIC_ASSERT(is_unsigned<T>::value);
  T res = 0;
  unsigned int bits = 0;
  for (; bits < sizeof(T) * 8 - 7; bits += 7)
  {
    uint8_t next;
    src.Read(&next, 1);
    res |= (static_cast<T>(next) & 127) << bits;
    if (!(next & 128))
      return res;
  }
  uint8_t next;
  src.Read(&next, 1);
  ASSERT_LESS_OR_EQUAL(next, (255 >> (sizeof(T) * 8 - bits)), ());
  ASSERT(!(next & 128), (next, bits));
  res |= static_cast<T>(next) << bits;
  return res
  */
}

template <typename T> inline typename make_unsigned<T>::type ZigZagEncode(T x)
{
  STATIC_ASSERT(is_signed<T>::value);
  return (x << 1) ^ (x >> (sizeof(x) * 8 - 1));
}

template <typename T> inline typename make_signed<T>::type ZigZagDecode(T x)
{
  STATIC_ASSERT(is_unsigned<T>::value);
  return (x >> 1) ^ -static_cast<typename make_signed<T>::type>(x & 1);
}

template <typename T, typename TSink> void WriteVarInt(TSink & dst, T value)
{
  STATIC_ASSERT(is_signed<T>::value);
  WriteVarUint(dst, ZigZagEncode(value));
}

template <typename T, typename TSource> T ReadVarInt(TSource & src)
{
  STATIC_ASSERT(is_signed<T>::value);
  return ZigZagDecode(ReadVarUint<typename make_unsigned<T>::type>(src));
}
