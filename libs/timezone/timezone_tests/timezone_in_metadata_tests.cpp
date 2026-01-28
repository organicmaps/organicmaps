#include <gtest/gtest.h>

#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "indexer/feature_meta.hpp"
#include "timezone/serdes.hpp"
#include "timezone/timezone.hpp"

using namespace om::tz;

TEST(TimeZoneInMetadata, ShouldCorrectlyStoreAndLoadTimeZone)
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

  feature::RegionData rd;
  rd.Set(feature::RegionData::RD_TIMEZONE, serialized);

  TimeZone deserialized;
  EXPECT_EQ(Deserialize(rd.Get(feature::RegionData::RD_TIMEZONE), deserialized), SerializationError::OK);
  EXPECT_EQ(tz, deserialized);

  std::vector<uint8_t> buf;

  // Serializing RegionMeta
  MemWriter writer(buf);
  rd.Serialize(writer);

  // Deserializing RegionMeta
  MemReader const reader(buf.data(), buf.size());
  ReaderSource src(reader);
  feature::RegionData rd2;
  rd2.Deserialize(src);

  deserialized = {};
  EXPECT_EQ(Deserialize(rd2.Get(feature::RegionData::RD_TIMEZONE), deserialized), SerializationError::OK);
  EXPECT_EQ(tz, deserialized);
}
