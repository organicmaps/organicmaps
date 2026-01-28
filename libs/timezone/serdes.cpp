#include "serdes.hpp"

#include "base/stl_helpers.hpp"
#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace om::tz
{
namespace
{
constexpr bool IsTimeZoneFormatVersionValid(TimeZoneFormatVersion const version)
{
  return base::E2I(version) == base::E2I(TimeZoneFormatVersion::Count) - 1;
}

constexpr bool IsGenerationYearOffsetValid(uint16_t const generationYearOffset)
{
  return generationYearOffset <= (1 << TimeZone::kGenerationYearBitSize) - 1;
}

constexpr bool IsBaseOffsetValid(uint8_t const baseOffset)
{
  return baseOffset <= (1 << TimeZone::kBaseOffsetBitSize) - 1;
}

constexpr bool IsDstDeltaValid(uint16_t const dstDelta)
{
  return dstDelta <= (1 << TimeZone::kDstDeltaBitSize) - 1;
}

constexpr bool IsTransitionsLengthValid(uint8_t const transitionsLength)
{
  return transitionsLength <= (1 << TimeZone::kTransitionsLengthBitSize) - 1;
}

constexpr bool IsDayDeltaValid(uint16_t const dayDelta)
{
  return dayDelta <= (1 << Transition::kDayDeltaBitSize) - 1;
}

constexpr bool IsMinuteOfDayValid(uint16_t const minuteOfDay)
{
  return minuteOfDay <= (1 << Transition::kMinuteOfDayBitSize) - 1;
}
}  // namespace

SerializationError Serialize(TimeZone const & timeZone, std::string & buf)
{
  MemWriter w(buf);
  BitWriter bw(w);

  if (!IsTimeZoneFormatVersionValid(timeZone.format_version))
    return SerializationError::UnsupportedTimeZoneFormat;
  bw.Write(base::E2I(timeZone.format_version), TimeZone::kFormatVersionBitSize);

  if (!IsGenerationYearOffsetValid(timeZone.generation_year_offset))
    return SerializationError::IncorrectGenerationYearOffsetFormat;
  bw.Write(timeZone.generation_year_offset, TimeZone::kGenerationYearBitSize);

  if (!IsBaseOffsetValid(timeZone.base_offset))
    return SerializationError::IncorrectBaseOffsetFormat;
  bw.Write(timeZone.base_offset, TimeZone::kBaseOffsetBitSize);

  if (!IsDstDeltaValid(timeZone.dst_delta))
    return SerializationError::IncorrectDstDeltaFormat;
  bw.WriteAtMost32Bits(timeZone.dst_delta, TimeZone::kDstDeltaBitSize);

  if (!IsTransitionsLengthValid(timeZone.transitions.size()))
    return SerializationError::IncorrectTransitionsLengthFormat;

  // The number of transitions must always be even. Otherwise, we may have a case with infinite dst
  if (timeZone.transitions.size() % 2 != 0)
    return SerializationError::IncorrectTransitionsAmount;

  bw.Write(timeZone.transitions.size(), TimeZone::kTransitionsLengthBitSize);

  for (auto [dayDelta, minuteOfDay] : timeZone.transitions)
  {
    if (!IsDayDeltaValid(dayDelta))
      return SerializationError::IncorrectDayDeltaFormat;
    bw.WriteAtMost32Bits(dayDelta, Transition::kDayDeltaBitSize);

    if (!IsMinuteOfDayValid(minuteOfDay))
      return SerializationError::IncorrectMinuteOfDayFormat;
    bw.WriteAtMost32Bits(minuteOfDay, Transition::kMinuteOfDayBitSize);
  }

  return SerializationError::OK;
}

SerializationError Deserialize(std::string_view const & data, TimeZone & tz)
{
  if (data.size() < TimeZone::kTotalSizeInBytes)
    return SerializationError::IncorrectHeader;

  MemReader const buf(data);
  ReaderSource src(buf);
  BitReader br{src};

  tz.format_version = static_cast<TimeZoneFormatVersion>(br.Read(TimeZone::kFormatVersionBitSize));
  if (!IsTimeZoneFormatVersionValid(tz.format_version))
    return SerializationError::UnsupportedTimeZoneFormat;

  tz.generation_year_offset = br.Read(TimeZone::kGenerationYearBitSize);
  tz.base_offset = static_cast<int16_t>(br.ReadAtMost32Bits(TimeZone::kBaseOffsetBitSize));
  tz.dst_delta = static_cast<int16_t>(br.ReadAtMost32Bits(TimeZone::kDstDeltaBitSize));
  size_t const transitionsLength = br.Read(TimeZone::kTransitionsLengthBitSize);
  tz.transitions.reserve(transitionsLength);

  size_t const expectedTimeZoneSizeInBytes =
      (TimeZone::kTotalSizeInBits + transitionsLength * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  if (data.size() < expectedTimeZoneSizeInBytes)
    return SerializationError::IncorrectTransitionsFormat;

  for (size_t i = 0; i < transitionsLength; ++i)
  {
    Transition t{};
    t.day_delta = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kDayDeltaBitSize));
    t.minute_of_day = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kMinuteOfDayBitSize));
    tz.transitions.push_back(t);
  }

  return SerializationError::OK;
}

std::string DebugPrint(SerializationError error)
{
  switch (error)
  {
    using enum SerializationError;
  case IncorrectHeader: return "IncorrectHeader";
  case IncorrectTransitionsFormat: return "IncorrectTransitionsFormat";
  case UnsupportedTimeZoneFormat: return "UnsupportedTimeZoneFormat";
  case IncorrectGenerationYearOffsetFormat: return "IncorrectGenerationYearOffsetFormat";
  case IncorrectBaseOffsetFormat: return "IncorrectBaseOffsetFormat";
  case IncorrectDstDeltaFormat: return "IncorrectDstDeltaFormat";
  case IncorrectTransitionsLengthFormat: return "IncorrectTransitionsLengthFormat";
  case IncorrectTransitionsAmount: return "IncorrectTransitionsAmount";
  case IncorrectDayDeltaFormat: return "IncorrectDayDeltaFormat";
  case IncorrectMinuteOfDayFormat: return "IncorrectMinuteOfDayFormat";
  default: UNREACHABLE();
  }
}
}  // namespace om::tz
