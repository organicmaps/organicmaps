#include <gtest/gtest.h>

#include "timezone/serdes.hpp"

using namespace om::tz;

TEST(TimeZone, Serialize1)
{
  TimeZone const tz{.generation_year_offset = 0, .base_offset = 0, .dst_delta = 0, .transitions = {}};

  std::string const serialized = Serialize(tz);
  TimeZone const deserialized = Deserialize(serialized);

  EXPECT_EQ(tz, deserialized);
}

TEST(TimeZone, Serialize2)
{
  TimeZone const tz{.generation_year_offset = 0,
                    .base_offset = 60,  // UTC+1
                    .dst_delta = 60,    // DST +1h
                    .transitions = {
                        {.day_delta = 88, .minute_of_day = 60, .is_dst = 1},
                        {.day_delta = 154, .minute_of_day = 60, .is_dst = 0},
                        {.day_delta = 210, .minute_of_day = 120, .is_dst = 1},
                        {.day_delta = 234, .minute_of_day = 120, .is_dst = 0},
                    }};

  std::string const serialized = Serialize(tz);
  TimeZone const deserialized = Deserialize(serialized);

  EXPECT_EQ(tz, deserialized);
}
