#pragma once

#include "routing/routing_exceptions.hpp"

#include "coding/bit_streams.hpp"
#include "coding/elias_coder.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"
#include "base/checked_cast.hpp"
#include "base/macros.hpp"

#include <limits>
#include <type_traits>

namespace routing
{
template <typename T, typename Source>
T ReadDelta(BitReader<Source> & reader)
{
  static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
  uint64_t const decoded = coding::DeltaCoder::Decode(reader);
  if (decoded > std::numeric_limits<T>::max())
    MYTHROW(CorruptedDataException, ("Decoded value", decoded, "out of limit", std::numeric_limits<T>::max()));

  return static_cast<T>(decoded);
}

template <typename T, typename Sink>
void WriteDelta(BitWriter<Sink> & writer, T value)
{
  static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
  ASSERT_GREATER(value, 0, ());

  bool const success = coding::DeltaCoder::Encode(writer, static_cast<uint64_t>(value));
  ASSERT(success, ());
  UNUSED_VALUE(success);
}

template <typename T, typename Source>
T ReadGamma(BitReader<Source> & reader)
{
  static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
  uint64_t const decoded = coding::GammaCoder::Decode(reader);
  if (decoded > std::numeric_limits<T>::max())
    MYTHROW(CorruptedDataException, ("Decoded value", decoded, "out of limit", std::numeric_limits<T>::max()));

  return static_cast<T>(decoded);
}

template <typename T, typename Sink>
void WriteGamma(BitWriter<Sink> & writer, T value)
{
  static_assert(std::is_unsigned<T>::value, "T should be an unsigned type");
  ASSERT_GREATER(value, 0, ());

  bool const success = coding::GammaCoder::Encode(writer, static_cast<uint64_t>(value));
  ASSERT(success, ());
  UNUSED_VALUE(success);
}

// C++ standard says:
// if the value can't be represented in the destination unsigned type,
// the result is implementation-defined.
//
// ModularCast makes unambiguous conversion from unsigned value to signed.
// The resulting value is the least signed integer congruent to the source integer
// (modulo 2^n where n is the number of bits used to represent the unsigned type)
template <typename Unsigned, typename Signed = std::make_signed_t<Unsigned>>
Signed ModularCast(Unsigned value)
{
  static_assert(std::is_unsigned<Unsigned>::value, "T should be an unsigned type");

  if (value <= static_cast<Unsigned>(std::numeric_limits<Signed>::max()))
    return static_cast<Signed>(value);

  auto constexpr minSignedT = std::numeric_limits<Signed>::min();
  return static_cast<Signed>(value - static_cast<Unsigned>(minSignedT)) + minSignedT;
}

// Encodes current as delta compared with prev.
template <typename T, typename UnsignedT = std::make_unsigned_t<T>>
UnsignedT EncodeZigZagDelta(T prev, T current)
{
  static_assert(std::is_integral<T>::value, "T should be an integral type");

  auto const unsignedPrev = static_cast<UnsignedT>(prev);
  auto const unsignedCurrent = static_cast<UnsignedT>(current);
  auto originalDelta = ModularCast(static_cast<UnsignedT>(unsignedCurrent - unsignedPrev));
  static_assert(std::is_same<decltype(originalDelta), std::make_signed_t<T>>::value,
                "It's expected that ModuleCast returns SignedT");

  auto encodedDelta = bits::ZigZagEncode(originalDelta);
  static_assert(std::is_same<decltype(encodedDelta), UnsignedT>::value,
                "It's expected that bits::ZigZagEncode returns UnsignedT");
  return encodedDelta;
}

// Reverse function for EncodeZigZagDelta.
template <typename T, typename UnsignedT = std::make_unsigned_t<T>>
T DecodeZigZagDelta(T prev, UnsignedT delta)
{
  static_assert(std::is_integral<T>::value, "T should be an integral type");

  auto decoded = bits::ZigZagDecode(delta);
  static_assert(std::is_same<decltype(decoded), std::make_signed_t<T>>::value,
                "It's expected that bits::ZigZagDecode returns SignedT");
  return prev + static_cast<T>(decoded);
}
}  // namespace routing
