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

std::expected<std::string, SerializationError> Serialize(TimeZone const & timeZone)
{
  std::string buf;
  {
    MemWriter w(buf);
    BitWriter bw(w);

    if (!IsTimeZoneFormatVersionValid(timeZone.format_version))
      return std::unexpected{SerializationError::UnsupportedTimeZoneFormat};
    bw.Write(base::E2I(timeZone.format_version), TimeZone::kFormatVersionBitSize);

    if (!IsGenerationYearOffsetValid(timeZone.generation_year_offset))
      return std::unexpected{SerializationError::IncorrectGenerationYearOffsetFormat};
    bw.Write(timeZone.generation_year_offset, TimeZone::kGenerationYearBitSize);

    if (!IsBaseOffsetValid(timeZone.base_offset))
      return std::unexpected{SerializationError::IncorrectBaseOffsetFormat};
    bw.Write(timeZone.base_offset, TimeZone::kBaseOffsetBitSize);

    if (!IsDstDeltaValid(timeZone.dst_delta))
      return std::unexpected{SerializationError::IncorrectDstDeltaFormat};
    bw.WriteAtMost32Bits(timeZone.dst_delta, TimeZone::kDstDeltaBitSize);

    if (!IsTransitionsLengthValid(timeZone.transitions_length))
      return std::unexpected{SerializationError::IncorrectTransitionsLengthFormat};

    // The number of transitions must always be even. Otherwise, we may have a case with infinite dst
    if (timeZone.transitions_length % 2 != 0)
      return std::unexpected{SerializationError::IncorrectTransitionsAmount};

    bw.Write(timeZone.transitions_length, TimeZone::kTransitionsLengthBitSize);

    for (int i = 0; i < timeZone.transitions_length; i++)
    {
      auto const [dayDelta, minuteOfDay] = timeZone.transitions[i];
      if (!IsDayDeltaValid(dayDelta))
        return std::unexpected{SerializationError::IncorrectDayDeltaFormat};
      bw.WriteAtMost32Bits(dayDelta, Transition::kDayDeltaBitSize);

      if (!IsMinuteOfDayValid(minuteOfDay))
        return std::unexpected{SerializationError::IncorrectMinuteOfDayFormat};
      bw.WriteAtMost32Bits(minuteOfDay, Transition::kMinuteOfDayBitSize);
    }
  }

  return buf;
}

std::expected<TimeZone, SerializationError> Deserialize(std::string_view const data)
{
  if (data.size() < TimeZone::kTotalSizeInBytes)
    return std::unexpected{SerializationError::IncorrectHeader};

  MemReader const buf(data);
  ReaderSource src(buf);
  BitReader br{src};
  TimeZone tz;

  tz.format_version = static_cast<TimeZoneFormatVersion>(br.Read(TimeZone::kFormatVersionBitSize));
  if (!IsTimeZoneFormatVersionValid(tz.format_version))
    return std::unexpected{SerializationError::UnsupportedTimeZoneFormat};

  tz.generation_year_offset = br.Read(TimeZone::kGenerationYearBitSize);
  tz.base_offset = static_cast<uint8_t>(br.ReadAtMost32Bits(TimeZone::kBaseOffsetBitSize));
  tz.dst_delta = static_cast<uint8_t>(br.ReadAtMost32Bits(TimeZone::kDstDeltaBitSize));
  tz.transitions_length = br.Read(TimeZone::kTransitionsLengthBitSize);

  size_t const expectedTimeZoneSizeInBytes =
      (TimeZone::kTotalSizeInBits + tz.transitions_length * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  if (data.size() < expectedTimeZoneSizeInBytes)
    return std::unexpected{SerializationError::IncorrectTransitionsFormat};

  for (size_t i = 0; i < tz.transitions_length; ++i)
  {
    auto & [dayDelta, minuteOfDay] = tz.transitions[i];
    dayDelta = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kDayDeltaBitSize));
    minuteOfDay = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kMinuteOfDayBitSize));
  }

  return tz;
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
