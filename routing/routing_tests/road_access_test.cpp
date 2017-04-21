#include "testing/testing.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/routing_tests/index_graph_tools.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

using namespace routing;
using namespace routing_test;
using namespace std;

namespace
{
UNIT_TEST(RoadAccess_Serialization)
{
  map<Segment, RoadAccess::Type> const m0 = {
      {Segment(kFakeNumMwmId, 1, 0, false), RoadAccess::Type::No},
      {Segment(kFakeNumMwmId, 2, 2, false), RoadAccess::Type::Private},
  };

  map<Segment, RoadAccess::Type> const m1 = {
      {Segment(kFakeNumMwmId, 1, 1, false), RoadAccess::Type::Private},
      {Segment(kFakeNumMwmId, 2, 0, true), RoadAccess::Type::Destination},
  };

  RoadAccess roadAccess;
  roadAccess.SetTypes(static_cast<RouterType>(0), m0);
  roadAccess.SetTypes(static_cast<RouterType>(1), m1);

  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    RoadAccessSerializer::Serialize(writer, roadAccess);
  }

  RoadAccess deserializedRoadAccess;
  {
    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());
  }

  TEST_EQUAL(roadAccess, deserializedRoadAccess, ());
}
}  // namespace
