#include "testing/testing.hpp"

#include "timezone/serdes.hpp"

#include "coding/bit_streams.hpp"
#include "coding/writer.hpp"

#include "base/stl_helpers.hpp"

using namespace om::tz;

UNIT_TEST(TimeZoneSerDes_EmptyTimeZone)
{
  TimeZone const tz{.generation_year_offset = 0, .base_offset = 0, .dst_delta = 0, .transitions = {}};

  auto const result = Serialize(tz);
  TEST(result.has_value(), ());
  std::string const & serialized = result.value();
  TEST_EQUAL(serialized.size(), TimeZone::kTotalSizeInBytes, ());

  auto const result2 = Deserialize(serialized);
  TEST(result2.has_value(), ());
  TimeZone const & deserialized = result2.value();
  TEST(tz == deserialized, ());
}

UNIT_TEST(TimeZoneSerDes_TimeZoneWithTransitions)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 68,  // UTC+1
                    .dst_delta = 60,    // DST +1h
                    .transitions = {
                        Transition{.day_delta = 88, .minute_of_day = 60},
                        Transition{.day_delta = 154, .minute_of_day = 60},
                        Transition{.day_delta = 210, .minute_of_day = 120},
                        Transition{.day_delta = 234, .minute_of_day = 120},
                    }};

  auto const result = Serialize(tz);
  TEST(result.has_value(), ());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  TEST_EQUAL(serialized.size(), expectedSizeInBytes, ());

  auto const result2 = Deserialize(serialized);
  TEST(result2.has_value(), ());
  TimeZone const & deserialized = result2.value();
  TEST(tz == deserialized, ());
}

UNIT_TEST(TimeZoneSerDes_StringView)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 68,  // UTC+1
                    .dst_delta = 60,    // DST +1h
                    .transitions = {
                        Transition{.day_delta = 88, .minute_of_day = 60},
                        Transition{.day_delta = 154, .minute_of_day = 60},
                        Transition{.day_delta = 210, .minute_of_day = 120},
                        Transition{.day_delta = 234, .minute_of_day = 120},
                    }};

  auto const result = Serialize(tz);
  TEST(result.has_value(), ());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  TEST_EQUAL(serialized.size(), expectedSizeInBytes, ());

  std::string const storage = "prefix" + serialized + "suffix";
  std::string_view const sv = std::string_view(storage).substr(6, serialized.size());
  auto const deserialized = Deserialize(sv);
  TEST(deserialized.has_value(), ());
  TEST(tz == deserialized.value(), ());
}

UNIT_TEST(TimeZoneSerDes_SerializeRejectsOddTransitions)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 68,
                    .dst_delta = 60,
                    .transitions = {
                        Transition{.day_delta = 88, .minute_of_day = 60},
                        Transition{.day_delta = 154, .minute_of_day = 60},
                        Transition{.day_delta = 210, .minute_of_day = 120},
                    }};

  auto const result = Serialize(tz);
  TEST(!result.has_value(), ());
  TEST_EQUAL(result.error(), SerializationError::IncorrectTransitionsAmount, ());
}

UNIT_TEST(TimeZoneSerDes_DeserializeRejectsOddTransitions)
{
  // Construct a raw blob with transitions_length = 3 (odd) using BitWriter.
  std::string buf;
  {
    MemWriter w(buf);
    BitWriter bw(w);
    bw.Write(base::E2I(TimeZoneFormatVersion::V1), TimeZone::kFormatVersionBitSize);
    bw.Write(0, TimeZone::kGenerationYearBitSize);
    bw.Write(68, TimeZone::kBaseOffsetBitSize);
    bw.WriteAtMost32Bits(60, TimeZone::kDstDeltaBitSize);
    bw.Write(3, TimeZone::kTransitionsLengthBitSize);  // odd!
    // Write 3 transitions so the size check passes.
    for (int i = 0; i < 3; ++i)
    {
      bw.WriteAtMost32Bits(88, Transition::kDayDeltaBitSize);
      bw.WriteAtMost32Bits(60, Transition::kMinuteOfDayBitSize);
    }
  }

  auto const result = Deserialize(buf);
  TEST(!result.has_value(), ());
  TEST_EQUAL(result.error(), SerializationError::IncorrectTransitionsAmount, ());
}

UNIT_TEST(TimeZoneSerDes_SerDesRealData)
{
  TimeZoneDb const & db = TimeZoneDb::Instance();
  for (auto const & [tzName, tz] : db.GetTimeZones())
  {
    auto const result = Serialize(tz);
    TEST(result.has_value(), (tzName));
    std::string const & serialized = result.value();
    auto const result2 = Deserialize(serialized);
    TEST(result2.has_value(), (tzName));
    TimeZone const & deserialized = result2.value();
    TEST(tz == deserialized, ("Error in", tzName, "ser/des"));
  }
}
