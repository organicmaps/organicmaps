#include <gtest/gtest.h>

#include "timezone/serdes.hpp"

#include "coding/bit_streams.hpp"
#include "coding/writer.hpp"

#include "base/stl_helpers.hpp"

TEST(TimeZoneSerDes, EmptyTimeZone)
{
  constexpr om::tz::TimeZone tz{.generation_year_offset = 0, .base_offset = 0, .dst_delta = 0, .transitions = {}};

  auto const result = om::tz::Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  EXPECT_EQ(serialized.size(), om::tz::TimeZone::kTotalSizeInBytes);

  auto const result2 = om::tz::Deserialize(serialized);
  EXPECT_TRUE(result2.has_value());
  om::tz::TimeZone const & deserialized = result2.value();
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, TimeZoneWithTransitions)
{
  om::tz::TimeZone const tz{.generation_year_offset = 0,
                            .base_offset = 68,  // UTC+1
                            .dst_delta = 60,    // DST +1h
                            .transitions = {
                                om::tz::Transition{.day_delta = 88, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 154, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 210, .minute_of_day = 120},
                                om::tz::Transition{.day_delta = 234, .minute_of_day = 120},
                            }};

  auto const result = om::tz::Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (om::tz::TimeZone::kTotalSizeInBits + 4 * om::tz::Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  auto const result2 = om::tz::Deserialize(serialized);
  EXPECT_TRUE(result2.has_value());
  om::tz::TimeZone const & deserialized = result2.value();
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, StringView)
{
  om::tz::TimeZone const tz{.generation_year_offset = 0,
                            .base_offset = 68,  // UTC+1
                            .dst_delta = 60,    // DST +1h
                            .transitions = {
                                om::tz::Transition{.day_delta = 88, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 154, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 210, .minute_of_day = 120},
                                om::tz::Transition{.day_delta = 234, .minute_of_day = 120},
                            }};

  auto const result = om::tz::Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (om::tz::TimeZone::kTotalSizeInBits + 4 * om::tz::Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  std::string_view const sv{serialized};
  EXPECT_EQ(sv.size(), serialized.size());
}

TEST(TimeZoneSerDes, SerializeRejectsOddTransitions)
{
  om::tz::TimeZone const tz{.generation_year_offset = 0,
                            .base_offset = 68,
                            .dst_delta = 60,
                            .transitions = {
                                om::tz::Transition{.day_delta = 88, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 154, .minute_of_day = 60},
                                om::tz::Transition{.day_delta = 210, .minute_of_day = 120},
                            }};

  auto const result = om::tz::Serialize(tz);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), om::tz::SerializationError::IncorrectTransitionsAmount);
}

TEST(TimeZoneSerDes, DeserializeRejectsOddTransitions)
{
  // Construct a raw blob with transitions_length = 3 (odd) using BitWriter.
  std::string buf;
  {
    MemWriter w(buf);
    BitWriter bw(w);
    bw.Write(base::E2I(om::tz::TimeZoneFormatVersion::V1), om::tz::TimeZone::kFormatVersionBitSize);
    bw.Write(0, om::tz::TimeZone::kGenerationYearBitSize);
    bw.Write(68, om::tz::TimeZone::kBaseOffsetBitSize);
    bw.WriteAtMost32Bits(60, om::tz::TimeZone::kDstDeltaBitSize);
    bw.Write(3, om::tz::TimeZone::kTransitionsLengthBitSize);  // odd!
    // Write 3 transitions so the size check passes.
    for (int i = 0; i < 3; ++i)
    {
      bw.WriteAtMost32Bits(88, om::tz::Transition::kDayDeltaBitSize);
      bw.WriteAtMost32Bits(60, om::tz::Transition::kMinuteOfDayBitSize);
    }
  }

  auto const result = om::tz::Deserialize(buf);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), om::tz::SerializationError::IncorrectTransitionsAmount);
}

TEST(TimeZoneSerDes, SerDesRealData)
{
  om::tz::TimeZoneDb const & db = om::tz::TimeZoneDb::Instance();
  for (auto const & [tzName, tz] : db.GetTimeZones())
  {
    auto const result = om::tz::Serialize(tz);
    EXPECT_TRUE(result.has_value());
    std::string const & serialized = result.value();
    auto const result2 = om::tz::Deserialize(serialized);
    EXPECT_TRUE(result2.has_value());
    om::tz::TimeZone const & deserialized = result2.value();
    EXPECT_EQ(tz, deserialized) << "Error in " << tzName << " ser/des";
  }
}
