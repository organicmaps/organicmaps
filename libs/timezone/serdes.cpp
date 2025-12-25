#include "serdes.hpp"

#include "base/logging.hpp"

namespace
{
struct BitWriter
{
  std::string out;
  uint8_t current = 0;
  uint8_t used = 0;

  void write_bits(uint32_t value, uint8_t bits)
  {
    while (bits--)
    {
      current |= (value & 1) << used;
      value >>= 1;
      used++;

      if (used == 8)
      {
        out.push_back(static_cast<char>(current));
        current = 0;
        used = 0;
      }
    }
  }

  std::string finish()
  {
    if (used > 0)
      out.push_back(static_cast<char>(current));
    return out;
  }
};

struct BitReader
{
  uint8_t const * data;
  size_t size;
  size_t byte_pos = 0;
  uint8_t bit_pos = 0;

  explicit BitReader(std::string_view const & input)
    : data(reinterpret_cast<uint8_t const *>(input.data()))
    , size(input.size())
  {}

  uint32_t read_bits(uint8_t const bits)
  {
    uint32_t value = 0;
    for (uint8_t i = 0; i < bits; ++i)
    {
      if (byte_pos >= size)
        throw std::runtime_error("BitReader: out of data");

      uint8_t bit = (data[byte_pos] >> bit_pos) & 1;
      value |= (bit << i);

      bit_pos++;
      if (bit_pos == 8)
      {
        bit_pos = 0;
        byte_pos++;
      }
    }
    return value;
  }
};
}  // namespace

namespace om::tz
{
std::string Serialize(TimeZone const & timeZone)
{
  BitWriter bw;
  bw.write_bits(timeZone.generation_year_offset, TimeZone::kGenerationYearBitSize);
  bw.write_bits(timeZone.base_offset, TimeZone::kBaseOffsetBitSize);
  bw.write_bits(timeZone.dst_delta, TimeZone::kDstDeltaBitSize);
  bw.write_bits(timeZone.transitions.size(), TimeZone::kTransitionsLengthBitSize);

  for (auto const & [day_delta, minute_of_day, is_dst] : timeZone.transitions)
  {
    bw.write_bits(day_delta, Transition::kDayDeltaBitSize);
    bw.write_bits(minute_of_day, Transition::kMinuteOfDayBitSize);
    bw.write_bits(is_dst ? 1 : 0, Transition::kIsDstBitSize);
  }

  return bw.finish();
}

TimeZone Deserialize(std::string_view const & data)
{
  LOG(LWARNING, ("Deserializing TimeZone from string:", data));
  BitReader br{data};
  TimeZone tz;

  tz.generation_year_offset = br.read_bits(TimeZone::kGenerationYearBitSize);
  tz.base_offset = static_cast<int16_t>(br.read_bits(TimeZone::kBaseOffsetBitSize));
  tz.dst_delta = static_cast<int16_t>(br.read_bits(TimeZone::kDstDeltaBitSize));
  size_t const transitionsLength = br.read_bits(TimeZone::kTransitionsLengthBitSize);
  tz.transitions.reserve(transitionsLength);
  for (size_t i = 0; i < transitionsLength; ++i)
  {
    Transition t{};
    t.day_delta = static_cast<uint16_t>(br.read_bits(Transition::kDayDeltaBitSize));
    t.minute_of_day = static_cast<uint16_t>(br.read_bits(Transition::kMinuteOfDayBitSize));
    t.is_dst = br.read_bits(Transition::kIsDstBitSize);
    tz.transitions.push_back(t);
  }

  return tz;
}
}  // namespace om::tz
