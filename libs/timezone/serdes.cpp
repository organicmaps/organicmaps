#include "serdes.hpp"

#include "coding/bit_streams.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace om::tz
{
namespace
{
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

    if (!IsGenerationYearOffsetValid(timeZone.generation_year_offset))
      return std::unexpected{SerializationError::IncorrectGenerationYearOffsetFormat};
    bw.Write(timeZone.generation_year_offset, TimeZone::kGenerationYearBitSize);

    if (!IsBaseOffsetValid(timeZone.base_offset))
      return std::unexpected{SerializationError::IncorrectBaseOffsetFormat};
    bw.Write(timeZone.base_offset, TimeZone::kBaseOffsetBitSize);

    if (!IsDstDeltaValid(timeZone.dst_delta))
      return std::unexpected{SerializationError::IncorrectDstDeltaFormat};
    bw.WriteAtMost32Bits(timeZone.dst_delta, TimeZone::kDstDeltaBitSize);

    if (!IsTransitionsLengthValid(timeZone.transitions.size()))
      return std::unexpected{SerializationError::IncorrectTransitionsLengthFormat};
    bw.Write(timeZone.transitions.size(), TimeZone::kTransitionsLengthBitSize);

    for (auto const & [day_delta, minute_of_day, is_dst] : timeZone.transitions)
    {
      if (!IsDayDeltaValid(day_delta))
        return std::unexpected{SerializationError::IncorrectDayDeltaFormat};
      bw.WriteAtMost32Bits(day_delta, Transition::kDayDeltaBitSize);

      if (!IsMinuteOfDayValid(minute_of_day))
        return std::unexpected{SerializationError::IncorrectMinuteOfDayFormat};
      bw.WriteAtMost32Bits(minute_of_day, Transition::kMinuteOfDayBitSize);

      bw.Write(is_dst ? 1 : 0, Transition::kIsDstBitSize);
    }
  }

  return buf;
}

std::expected<TimeZone, SerializationError> Deserialize(std::string_view const & data)
{
  if (data.size() < TimeZone::kTotalSizeInBytes)
    return std::unexpected{SerializationError::IncorrectHeader};

  MemReader const buf(data);
  ReaderSource src(buf);
  BitReader br{src};
  TimeZone tz;

  tz.generation_year_offset = br.Read(TimeZone::kGenerationYearBitSize);
  tz.base_offset = static_cast<int16_t>(br.ReadAtMost32Bits(TimeZone::kBaseOffsetBitSize));
  tz.dst_delta = static_cast<int16_t>(br.ReadAtMost32Bits(TimeZone::kDstDeltaBitSize));
  size_t const transitionsLength = br.Read(TimeZone::kTransitionsLengthBitSize);
  tz.transitions.reserve(transitionsLength);

  size_t const expectedTimeZoneSizeInBytes =
      (TimeZone::kTotalSizeInBits + transitionsLength * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  if (data.size() < expectedTimeZoneSizeInBytes)
    return std::unexpected{SerializationError::IncorrectTransitionsFormat};

  for (size_t i = 0; i < transitionsLength; ++i)
  {
    Transition t{};
    t.day_delta = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kDayDeltaBitSize));
    t.minute_of_day = static_cast<uint16_t>(br.ReadAtMost32Bits(Transition::kMinuteOfDayBitSize));
    t.is_dst = br.Read(Transition::kIsDstBitSize);
    tz.transitions.push_back(t);
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
  case IncorrectGenerationYearOffsetFormat: return "IncorrectGenerationYearOffsetFormat";
  case IncorrectBaseOffsetFormat: return "IncorrectBaseOffsetFormat";
  case IncorrectDstDeltaFormat: return "IncorrectDstDeltaFormat";
  case IncorrectTransitionsLengthFormat: return "IncorrectTransitionsLengthFormat";
  case IncorrectDayDeltaFormat: return "IncorrectDayDeltaFormat";
  case IncorrectMinuteOfDayFormat: return "IncorrectMinuteOfDayFormat";
  default: UNREACHABLE();
  }
}
}  // namespace om::tz
