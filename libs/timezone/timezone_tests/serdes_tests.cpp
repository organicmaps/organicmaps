#include <gtest/gtest.h>

#include "timezone/serdes.hpp"

using namespace om::tz;

TEST(TimeZoneSerDes, EmptyTimeZone)
{
  TimeZone const tz{.generation_year_offset = 0, .base_offset = 0, .dst_delta = 0, .transitions = {}};

  std::string serialized;
  EXPECT_EQ(Serialize(tz, serialized), SerializationError::OK);
  EXPECT_EQ(serialized.size(), TimeZone::kTotalSizeInBytes);

  TimeZone deserialized;
  EXPECT_EQ(Deserialize(serialized, deserialized), SerializationError::OK);
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, TimeZoneWithTransitions)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 68,  // UTC+1
                    .dst_delta = 60,    // DST +1h
                    .transitions = {
                        {.day_delta = 88, .minute_of_day = 60},
                        {.day_delta = 154, .minute_of_day = 60},
                        {.day_delta = 210, .minute_of_day = 120},
                        {.day_delta = 234, .minute_of_day = 120},
                    }};

  std::string serialized;
  EXPECT_EQ(Serialize(tz, serialized), SerializationError::OK);

  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  TimeZone deserialized;
  EXPECT_EQ(Deserialize(serialized, deserialized), SerializationError::OK);
  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZoneSerDes, StringView)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 68,  // UTC+1
                    .dst_delta = 60,    // DST +1h
                    .transitions = {
                        {.day_delta = 88, .minute_of_day = 60},
                        {.day_delta = 154, .minute_of_day = 60},
                        {.day_delta = 210, .minute_of_day = 120},
                        {.day_delta = 234, .minute_of_day = 120},
                    }};

  std::string serialized;
  EXPECT_EQ(Serialize(tz, serialized), SerializationError::OK);

  constexpr size_t expectedSizeInBytes =
      (TimeZone::kTotalSizeInBits + 4 * Transition::kTotalSizeInBits + CHAR_BIT - 1) / CHAR_BIT;
  EXPECT_EQ(serialized.size(), expectedSizeInBytes);

  std::string_view const sv{serialized};
  EXPECT_EQ(sv.size(), serialized.size());
}
