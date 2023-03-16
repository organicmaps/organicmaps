#pragma once

#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/bits.hpp"
#include "base/exception.hpp"
#include "base/stl_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/// This function writes, using optimal bytes count.
/// Pass any integral type and it will write platform-independent.
template <typename T, typename TSink>
void WriteVarUint(TSink & dst, T value)
{
  static_assert(std::is_unsigned<T>::value, "");
  while (value > 127)
  {
    WriteToSink(dst, static_cast<uint8_t>((value & 127) | 128));
    value >>= 7;
  }
  WriteToSink(dst, static_cast<uint8_t>(value));
}

namespace impl
{
template <typename TSource>
uint32_t ReadVarUint(TSource & src, uint32_t const *)
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

template <typename TSource>
uint64_t ReadVarUint(TSource & src, uint64_t const *)
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

}  // namespace impl

template <typename T, typename TSource>
T ReadVarUint(TSource & src)
{
  static_assert((std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value), "");
  return ::impl::ReadVarUint(src, static_cast<T const *>(NULL));

  /* Generic code commented out.
  static_assert(is_unsigned<T>::value, "");
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

template <typename T, typename TSink>
void WriteVarInt(TSink & dst, T value)
{
  static_assert(std::is_signed<T>::value, "");
  WriteVarUint(dst, bits::ZigZagEncode(value));
}

template <typename T, typename TSource>
T ReadVarInt(TSource & src)
{
  static_assert(std::is_signed<T>::value, "");
  return bits::ZigZagDecode(ReadVarUint<std::make_unsigned_t<T>>(src));
}

DECLARE_EXCEPTION(ReadVarIntException, RootException);

namespace impl
{

class ReadVarInt64ArrayUntilBufferEnd
{
public:
  explicit ReadVarInt64ArrayUntilBufferEnd(void const * pEnd) : m_pEnd(pEnd) {}
  bool Continue(void const * p) const
  {
    ASSERT_LESS_OR_EQUAL(reinterpret_cast<uintptr_t>(p), reinterpret_cast<uintptr_t>(m_pEnd), ());
    return p < m_pEnd;
  }
  void NextVarInt() {}
private:
  void const * m_pEnd;
};

class ReadVarInt64ArrayGivenSize
{
public:
  explicit ReadVarInt64ArrayGivenSize(size_t const count) : m_Remaining(count) {}
  bool Continue(void const *) const { return m_Remaining > 0; }
  void NextVarInt() { --m_Remaining; }
private:
  size_t m_Remaining;
};

template <typename ConverterT, typename F, class WhileConditionT>
void const * ReadVarInt64Array(void const * pBeg, WhileConditionT whileCondition, F f,
                               ConverterT converter)
{
  uint8_t const * const pBegChar = static_cast<uint8_t const *>(pBeg);
  uint64_t res64 = 0;
  uint32_t res32 = 0;
  uint32_t count32 = 0;
  uint32_t count64 = 0;
  uint8_t const * p = pBegChar;
  while (whileCondition.Continue(p))
  {
    uint8_t const t = *p++;
    res32 += (static_cast<uint32_t>(t & 127) << count32);
    count32 += 7;
    if (!(t & 128))
    {
      f(converter((static_cast<uint64_t>(res32) << count64) + res64));
      whileCondition.NextVarInt();
      res64 = 0;
      res32 = 0;
      count32 = 0;
      count64 = 0;
    }
    else if (count32 == 28)
    {
      res64 += (static_cast<uint64_t>(res32) << count64);
      res32 = 0;
      count32 = 0;
      count64 += 28;
    }
  }
  ASSERT(count32 == 0 && res32 == 0 && res64 == 0, (res64, res32, count32));
  if (count32 != 0)
    MYTHROW(ReadVarIntException, ());
  return p;
}

}

template <typename F>
void const * ReadVarInt64Array(void const * pBeg, void const * pEnd, F f)
{
  return ::impl::ReadVarInt64Array<int64_t (*)(uint64_t)>(
        pBeg, ::impl::ReadVarInt64ArrayUntilBufferEnd(pEnd), f, &bits::ZigZagDecode);
}

template <typename F>
void const * ReadVarUint64Array(void const * pBeg, void const * pEnd, F f)
{
  return ::impl::ReadVarInt64Array(pBeg, ::impl::ReadVarInt64ArrayUntilBufferEnd(pEnd), f, base::IdFunctor());
}

template <typename F>
void const * ReadVarInt64Array(void const * pBeg, size_t count, F f)
{
  return ::impl::ReadVarInt64Array<int64_t (*)(uint64_t)>(
        pBeg, ::impl::ReadVarInt64ArrayGivenSize(count), f, &bits::ZigZagDecode);
}

template <typename F>
void const * ReadVarUint64Array(void const * pBeg, size_t count, F f)
{
  return ::impl::ReadVarInt64Array(pBeg, ::impl::ReadVarInt64ArrayGivenSize(count), f, base::IdFunctor());
}

template <class Cont, class Sink>
void WriteVarUintArray(Cont const & v, Sink & sink)
{
  for (size_t i = 0; i != v.size(); ++i)
    WriteVarUint(sink, v[i]);
}
