#include "testing/testing.hpp"

#include "track_analyzing/track_analyzer/utils.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing/segment.hpp"

#include "traffic/speed_groups.hpp"

#include "platform/country_file.hpp"

#include "geometry/latlon.hpp"

#include <string>

namespace
{
using namespace platform;
using namespace routing;
using namespace storage;
using namespace track_analyzing;
using namespace traffic;

UNIT_TEST(StatTest)
{
  Stats stats1 = {
      {{"Belarus_Minsk Region", 1}, {"Uzbekistan", 7}, {"Russia_Moscow", 5} /* Mwm to number */},
      {{"Russian Federation", 10}, {"Poland", 5} /* Country to number */}};

  Stats const stats2 = {{{"Belarus_Minsk Region", 2} /* Mwm to number */},
                        {{"Russian Federation", 1}, {"Belarus", 8} /* Country to number */}};

  stats1.Add(stats2);

  Stats const expected = {
      {{"Belarus_Minsk Region", 3}, {"Uzbekistan", 7}, {"Russia_Moscow", 5} /* Mwm to number */},
      {{"Russian Federation", 11}, {"Poland", 5}, {"Belarus", 8} /* Country to number */}};

  TEST_EQUAL(stats1, expected, ());
}

UNIT_TEST(AddStatTest)
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

  Stats stat;
  AddStat(mwmToTracks, *numMwmIds, storage, stat);

  Stats::NameToCountMapping const expectedMwmToTotalDataMapping = {{kMwmName, kDataPointNumber}};
  TEST_EQUAL(stat.m_mwmToTotalDataPoints, expectedMwmToTotalDataMapping, ());

  Stats::NameToCountMapping expectedCountryToTotalDataMapping = {{"Italy", kDataPointNumber}};
  TEST_EQUAL(stat.m_countryToTotalDataPoints, expectedCountryToTotalDataMapping, ());
}
}  // namespace
