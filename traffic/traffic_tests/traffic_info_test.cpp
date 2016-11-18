#include "testing/testing.hpp"

#include "traffic/speed_groups.hpp"
#include "traffic/traffic_info.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/mwm_set.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "std/algorithm.hpp"
#include "std/random.hpp"

namespace traffic
{
namespace
{
string const & kMapTestDir = "traffic-test";

SpeedGroup GetSpeedGroup(TrafficInfo::Coloring const & coloring,
                         TrafficInfo::RoadSegmentId const & fid)
{
  auto const it = coloring.find(fid);
  if (it == coloring.cend())
    return SpeedGroup::Unknown;
  return it->second;
}

class TestMwmSet : public MwmSet
{
protected:
  // MwmSet overrides:
  unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override
  {
    unique_ptr<MwmInfo> info(new MwmInfo());
    info->m_version.SetFormat(version::Format::lastFormat);
    return info;
  }

  unique_ptr<MwmValueBase> CreateValue(MwmInfo &) const override
  {
    return make_unique<MwmValueBase>();
  }
};
}  // namespace

UNIT_TEST(TrafficInfo_RemoteFile)
{
  WritableDirChanger writableDirChanger(kMapTestDir);
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test"));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    TEST(trafficInfo.ReceiveTrafficData(), ());
  }
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test2"));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    TEST(!trafficInfo.ReceiveTrafficData(), ());
  }
  {
    TestMwmSet mwmSet;
    auto const & r =
        mwmSet.Register(platform::LocalCountryFile::MakeForTesting("traffic_data_test", 101010));
    TrafficInfo trafficInfo(r.first, r.first.GetInfo()->GetVersion());
    TEST(trafficInfo.ReceiveTrafficData(), ());
  }
}

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

    ASSERT(is_sorted(keys.begin(), keys.end()), ());
    ASSERT(is_sorted(deserializedKeys.begin(), deserializedKeys.end()), ());
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
}  // namespace traffic
