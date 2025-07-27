#include "testing/testing.hpp"

#include "track_analyzing/track_analyzer/utils.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing/segment.hpp"

#include "traffic/speed_groups.hpp"

#include "platform/country_file.hpp"

#include "geometry/latlon.hpp"

#include <sstream>
#include <string>

namespace
{
using namespace platform;
using namespace routing;
using namespace storage;
using namespace track_analyzing;
using namespace traffic;

void TestSerializationToCsv(Stats::NameToCountMapping const & mapping)
{
  std::stringstream ss;
  MappingToCsv("mwm", mapping, false /* printPercentage */, ss);

  Stats::NameToCountMapping readMapping;
  MappingFromCsv(ss, readMapping);

  TEST_EQUAL(mapping, readMapping, (ss.str()));
}

UNIT_TEST(AddDataPointsTest)
{
  Stats stats;
  stats.AddDataPoints("mwm1", "country1", 3);
  stats.AddDataPoints("mwm1", "country1", 1);
  stats.AddDataPoints("mwm2", "country1", 5);
  stats.AddDataPoints("mwm3", "country3", 7);

  Stats const expected = {{{"mwm1", 4}, {"mwm2", 5}, {"mwm3", 7} /* Mwm to number */},
                          {{"country1", 9}, {"country3", 7} /* Country to number */}};

  TEST_EQUAL(stats, expected, ());
}

UNIT_TEST(AddStatTest)
{
  Stats stats1 = {{{"Belarus_Minsk Region", 1}, {"Uzbekistan", 7}, {"Russia_Moscow", 5} /* Mwm to number */},
                  {{"Russian Federation", 10}, {"Poland", 5} /* Country to number */}};

  Stats const stats2 = {{{"Belarus_Minsk Region", 2} /* Mwm to number */},
                        {{"Russian Federation", 1}, {"Belarus", 8} /* Country to number */}};

  stats1.Add(stats2);

  Stats const expected = {{{"Belarus_Minsk Region", 3}, {"Uzbekistan", 7}, {"Russia_Moscow", 5} /* Mwm to number */},
                          {{"Russian Federation", 11}, {"Poland", 5}, {"Belarus", 8} /* Country to number */}};

  TEST_EQUAL(stats1, expected, ());
}

UNIT_TEST(AddTracksStatsTest)
{
  DataPoint const dp1(1 /* timestamp */, ms::LatLon(), static_cast<uint8_t>(SpeedGroup::G5));
  DataPoint const dp2(2 /* timestamp */, ms::LatLon(), static_cast<uint8_t>(SpeedGroup::G5));
  DataPoint const dp3(3 /* timestamp */, ms::LatLon(), static_cast<uint8_t>(SpeedGroup::G5));
  DataPoint const dp4(4 /* timestamp */, ms::LatLon(), static_cast<uint8_t>(SpeedGroup::G5));

  uint32_t constexpr kDataPointNumber = 4;

  std::string const kMwmName = "Italy_Sardinia";

  Track const track1 = {dp1, dp2};
  Track const track2 = {dp3};
  Track const track3 = {dp4};

  UserToTrack const userToTrack = {{"id1", track1}, {"id2", track2}, {"id3", track3}};

  Storage storage;
  auto numMwmIds = CreateNumMwmIds(storage);
  MwmToTracks const mwmToTracks = {{numMwmIds->GetId(CountryFile(kMwmName)), userToTrack}};

  Stats stats;
  stats.AddTracksStats(mwmToTracks, *numMwmIds, storage);

  Stats::NameToCountMapping const expectedMwmToTotalDataMapping = {{kMwmName, kDataPointNumber}};
  TEST_EQUAL(stats.GetMwmToTotalDataPointsForTesting(), expectedMwmToTotalDataMapping, ());

  Stats::NameToCountMapping expectedCountryToTotalDataMapping = {{"Italy", kDataPointNumber}};
  TEST_EQUAL(stats.GetCountryToTotalDataPointsForTesting(), expectedCountryToTotalDataMapping, ());
}

UNIT_TEST(MappingToCsvTest)
{
  Stats::NameToCountMapping const mapping = {{{"Belarus_Minsk Region", 2}, {"Uzbekistan", 5}, {"Russia_Moscow", 3}}};
  {
    std::ostringstream ss;
    MappingToCsv("mwm", mapping, true /* printPercentage */, ss);
    std::string const expected = R"(mwm,number,percent
Uzbekistan,5,50
Russia_Moscow,3,30
Belarus_Minsk Region,2,20
)";
    TEST_EQUAL(ss.str(), expected, ());
  }
  {
    std::ostringstream ss;
    MappingToCsv("mwm", mapping, false /* printPercentage */, ss);
    std::string const expected = R"(mwm,number
Uzbekistan,5
Russia_Moscow,3
Belarus_Minsk Region,2
)";
    TEST_EQUAL(ss.str(), expected, ());
  }
}

UNIT_TEST(MappingToCsvUint64Test)
{
  Stats::NameToCountMapping const mapping = {{"Belarus_Minsk Region", 5'000'000'000}, {"Uzbekistan", 15'000'000'000}};
  std::stringstream ss;
  MappingToCsv("mwm", mapping, true /* printPercentage */, ss);
  std::string const expected = R"(mwm,number,percent
Uzbekistan,15000000000,75
Belarus_Minsk Region,5000000000,25
)";
  TEST_EQUAL(ss.str(), expected, ());
}

UNIT_TEST(SerializationToCsvTest)
{
  Stats::NameToCountMapping const mapping1 = {
      {"Belarus_Minsk Region", 2}, {"Uzbekistan", 5}, {"Russia_Moscow", 1}, {"Russia_Moscow Oblast_East", 2}};
  TestSerializationToCsv(mapping1);

  Stats::NameToCountMapping const mapping2 = {{{"Belarus_Minsk Region", 2}, {"Uzbekistan", 5}, {"Russia_Moscow", 3}}};
  TestSerializationToCsv(mapping2);
}

UNIT_TEST(SerializationToCsvWithZeroValueTest)
{
  Stats::NameToCountMapping const mapping = {{"Russia_Moscow Oblast_East", 2}, {"Poland_Lesser Poland Voivodeship", 0}};
  Stats::NameToCountMapping const expected = {{"Russia_Moscow Oblast_East", 2}};

  std::stringstream ss;
  MappingToCsv("mwm", mapping, false /* printPercentage */, ss);

  Stats::NameToCountMapping readMapping;
  MappingFromCsv(ss, readMapping);

  TEST_EQUAL(readMapping, expected, (ss.str()));
}

UNIT_TEST(SerializationToCsvUint64Test)
{
  Stats::NameToCountMapping const mapping = {
      {"Belarus_Minsk Region", 20'000'000'000}, {"Uzbekistan", 5}, {"Russia_Moscow", 7'000'000'000}};
  TestSerializationToCsv(mapping);
}
}  // namespace
