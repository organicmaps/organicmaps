#include <gtest/gtest.h>

#include "timezone/serdes.hpp"

using namespace om::tz;

TEST(TimeZoneSerDes, EmptyTimeZone)
{
  constexpr TimeZone tz{
      .generation_year_offset = 0, .base_offset = 0, .dst_delta = 0, .transitions_length = 0, .transitions = {}};

  auto const result = Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  EXPECT_EQ(serialized.size(), TimeZone::kTotalSizeInBytes);

  auto const result2 = Deserialize(serialized);
  EXPECT_TRUE(result2.has_value());
  TimeZone const & deserialized = result2.value();
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, TimeZoneWithTransitions)
{
  constexpr TimeZone tz{.generation_year_offset = 0,
                        .base_offset = 68,  // UTC+1
                        .dst_delta = 60,    // DST +1h
                        .transitions_length = 4,
                        .transitions = {
                            Transition{.day_delta = 88, .minute_of_day = 60},
                            Transition{.day_delta = 154, .minute_of_day = 60},
                            Transition{.day_delta = 210, .minute_of_day = 120},
                            Transition{.day_delta = 234, .minute_of_day = 120},
                        }};

  auto const result = Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  auto const result2 = Deserialize(serialized);
  EXPECT_TRUE(result2.has_value());
  TimeZone const & deserialized = result2.value();
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, StringView)
{
  constexpr TimeZone tz{.generation_year_offset = 0,
                        .base_offset = 68,  // UTC+1
                        .dst_delta = 60,    // DST +1h
                        .transitions_length = 4,
                        .transitions = {
                            Transition{.day_delta = 88, .minute_of_day = 60},
                            Transition{.day_delta = 154, .minute_of_day = 60},
                            Transition{.day_delta = 210, .minute_of_day = 120},
                            Transition{.day_delta = 234, .minute_of_day = 120},
                        }};

  auto const result = Serialize(tz);
  EXPECT_TRUE(result.has_value());
  std::string const & serialized = result.value();
  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  std::string_view const sv{serialized};
  EXPECT_EQ(sv.size(), serialized.size());
}
