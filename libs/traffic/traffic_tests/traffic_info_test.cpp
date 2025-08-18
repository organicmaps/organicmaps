#include "testing/testing.hpp"

#include "traffic/speed_groups.hpp"
#include "traffic/traffic_info.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "indexer/mwm_set.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace std;

namespace traffic
{
namespace
{
string const & kMapTestDir = "traffic-test";

class TestMwmSet : public MwmSet
{
protected:
  // MwmSet overrides:
  unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const &) const override
  {
    unique_ptr<MwmInfo> info(new MwmInfo());
    info->m_version.SetFormat(version::Format::lastFormat);
    return info;
  }

  unique_ptr<MwmValue> CreateValue(MwmInfo & info) const override { return make_unique<MwmValue>(info.GetLocalFile()); }
};
}  // namespace

/// @todo Need TRAFFIC_DATA_BASE_URL for this test.
/*
UNIT_TEST(TrafficInfo_RemoteFile)
{
  WritableDirChanger writableDirChanger(kMapTestDir);
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test"));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    string etag;
    TEST(trafficInfo.ReceiveTrafficData(etag), ());
  }
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test2"));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    string etag;
    TEST(!trafficInfo.ReceiveTrafficData(etag), ());
  }
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test", 101010));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    string etag;
    TEST(trafficInfo.ReceiveTrafficData(etag), ());
  }
}
*/

UNIT_TEST(TrafficInfo_Serialization)
{
  TrafficInfo::Coloring coloring = {
      {TrafficInfo::RoadSegmentId(0, 0, 0), SpeedGroup::G0},

      {TrafficInfo::RoadSegmentId(1, 0, 0), SpeedGroup::G1},
      {TrafficInfo::RoadSegmentId(1, 0, 1), SpeedGroup::G3},

      {TrafficInfo::RoadSegmentId(5, 0, 0), SpeedGroup::G2},
      {TrafficInfo::RoadSegmentId(5, 0, 1), SpeedGroup::G2},
      {TrafficInfo::RoadSegmentId(5, 1, 0), SpeedGroup::G2},
      {TrafficInfo::RoadSegmentId(5, 1, 1), SpeedGroup::G5},

      {TrafficInfo::RoadSegmentId(4294967295, 0, 0), SpeedGroup::TempBlock},
  };

  vector<TrafficInfo::RoadSegmentId> keys;
  vector<SpeedGroup> values;
  for (auto const & kv : coloring)
  {
    keys.push_back(kv.first);
    values.push_back(kv.second);
  }

  {
    vector<uint8_t> buf;
    TrafficInfo::SerializeTrafficKeys(keys, buf);

    vector<TrafficInfo::RoadSegmentId> deserializedKeys;
    TrafficInfo::DeserializeTrafficKeys(buf, deserializedKeys);

    TEST(is_sorted(keys.begin(), keys.end()), ());
    TEST(is_sorted(deserializedKeys.begin(), deserializedKeys.end()), ());
    TEST_EQUAL(keys, deserializedKeys, ());
  }

  {
    vector<uint8_t> buf;
    TrafficInfo::SerializeTrafficValues(values, buf);

    vector<SpeedGroup> deserializedValues;
    TrafficInfo::DeserializeTrafficValues(buf, deserializedValues);
    TEST_EQUAL(values, deserializedValues, ());
  }
}

UNIT_TEST(TrafficInfo_UpdateTrafficData)
{
  vector<TrafficInfo::RoadSegmentId> const keys = {
      TrafficInfo::RoadSegmentId(0, 0, 0),

      TrafficInfo::RoadSegmentId(1, 0, 0),
      TrafficInfo::RoadSegmentId(1, 0, 1),
  };

  vector<SpeedGroup> const values1 = {
      SpeedGroup::G1,
      SpeedGroup::G2,
      SpeedGroup::G3,
  };

  vector<SpeedGroup> const values2 = {
      SpeedGroup::G4,
      SpeedGroup::G5,
      SpeedGroup::Unknown,
  };

  TrafficInfo info;
  info.SetTrafficKeysForTesting(keys);

  TEST(info.UpdateTrafficData(values1), ());
  for (size_t i = 0; i < keys.size(); ++i)
    TEST_EQUAL(info.GetSpeedGroup(keys[i]), values1[i], ());

  TEST(info.UpdateTrafficData(values2), ());
  for (size_t i = 0; i < keys.size(); ++i)
    TEST_EQUAL(info.GetSpeedGroup(keys[i]), values2[i], ());
}
}  // namespace traffic
