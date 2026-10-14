#include "testing/testing.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"
#include "indexer/feature_meta.hpp"
#include "timezone/serdes.hpp"
#include "timezone/timezone.hpp"

using namespace om::tz;

UNIT_TEST(TimeZoneInMetadata_ShouldCorrectlyStoreAndLoadTimeZone)
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

  feature::RegionData rd;
  auto const result = Serialize(tz);
  TEST(result.has_value(), ());
  rd.Set(feature::RegionData::RD_TIMEZONE, result.value());
  auto const stored = Deserialize(rd.Get(feature::RegionData::RD_TIMEZONE));
  TEST(stored.has_value(), ());
  TEST(tz == stored.value(), ());

  std::vector<uint8_t> buf;

  // Serializing RegionMeta
  MemWriter writer(buf);
  rd.Serialize(writer);

  // Deserializing RegionMeta
  MemReader const reader(buf.data(), buf.size());
  ReaderSource src(reader);
  feature::RegionData rd2;
  rd2.Deserialize(src);
  auto const restored = Deserialize(rd2.Get(feature::RegionData::RD_TIMEZONE));
  TEST(restored.has_value(), ());
  TEST(tz == restored.value(), ());
}
