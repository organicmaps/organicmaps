#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

namespace coding_utils
{
namespace detail
{
// WriteVarIntegral is abstraction over WriteVarUint and WriteVarInt functions.
// WriteVarIntegral may be used in generic code.
template <
    typename T, typename Sink,
    typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, int> = 0>
void WriteVarIntegral(Sink & dst, T value)
{
  WriteVarUint(dst, value);
}
template <
    typename T, typename Sink,
    typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, int> = 0>
void WriteVarIntegral(Sink & dst, T value)
{
  WriteVarInt(dst, value);
}

// ReadVarIntegral is abstraction over ReadVarUint and ReadVarInt functions.
// ReadVarIntegral may be used in generic code.
template <
    typename T, typename Source,
    typename std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, int> = 0>
T ReadVarIntegral(Source & src)
{
  return ReadVarUint<T>(src);
}
template <
    typename T, typename Source,
    typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, int> = 0>
T ReadVarIntegral(Source & src)
{
  return ReadVarInt<T>(src);
}
}  // namespace detail

// Type of collection size. It used for reading and writing collections.
using CollectionSizeType = uint32_t;

// DeltaEncodeAs encodes data in the form of differences between sequential data.
// It required sorted |container|. |fn| may used used to access structure fields or data modification.
template <typename ValueType, typename Sink, typename Cont, typename Fn>
void DeltaEncodeAs(Sink & sink, Cont const & container, Fn && fn)
{
  ASSERT((std::is_sorted(std::cbegin(container), std::cend(container),
                         [&](auto const & lhs, auto const & rhs) { return fn(lhs) < fn(rhs); })),
         ());

  auto const contSize = base::checked_cast<CollectionSizeType>(container.size());
  WriteVarUint(sink, contSize);
  if (contSize == 0)
    return;

  auto first = std::begin(container);
  auto const last = std::end(container);
  auto acc = fn(*first);
  detail::WriteVarIntegral(sink, acc);
  while (++first != last)
  {
    auto const val = fn(*first);
    auto const delta = base::checked_cast<uint64_t>(val - acc);
    WriteVarUint(sink, delta);
    acc = val;
  }
}

// DeltaDecodeAs decodes data from the form of differences between sequential data.
// |fn| may used used to initialize an object or data modification.
template <typename ValueType, typename Source, typename OutIt, typename Fn>
void DeltaDecodeAs(Source & src, OutIt it, Fn && fn)
{
  auto contSize = ReadVarUint<CollectionSizeType>(src);
  if (contSize == 0)
    return;

  auto sum = detail::ReadVarIntegral<ValueType>(src);
  *it++ = fn(sum);
  while (--contSize)
  {
    sum += base::checked_cast<ValueType>(ReadVarUint<uint64_t>(src));
    *it++ = fn(sum);
  }
}

template <typename Sink, typename Cont, typename ValueType = typename Cont::value_type>
void DeltaEncode(Sink & sink, Cont const & container, ValueType base = {})
{
  DeltaEncodeAs<ValueType>(sink, container, [&](ValueType val) { return val - base; });
}

template <typename Source, typename OutIt, typename ValueType = typename OutIt::container_type::value_type>
void DeltaDecode(Source & src, OutIt it, ValueType base = {})
{
  DeltaDecodeAs<ValueType>(src, it, [&](ValueType val) { return val + base; });
}

// WriteCollectionPrimitive writes collection. It used WriteToSink function.
template <typename Sink, typename Cont>
void WriteCollectionPrimitive(Sink & sink, Cont const & container)
{
  auto const contSize = base::checked_cast<CollectionSizeType>(container.size());
  WriteVarUint(sink, contSize);
  for (auto value : container)
    WriteToSink(sink, value);
}

// ReadCollectionPrimitive reads collection. It used ReadPrimitiveFromSource function.
template <typename Source, typename OutIt>
void ReadCollectionPrimitive(Source & src, OutIt it)
{
  using ValueType = typename OutIt::container_type::value_type;

  auto size = ReadVarUint<CollectionSizeType>(src);
  while (size--)
    *it++ = ReadPrimitiveFromSource<ValueType>(src);
}
}  // namespace coding_utils
