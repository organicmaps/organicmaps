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
      {Segment(kFakeNumMwmId, 1 /* feature id */, 0 /* segment idx */, false /* forward */),
       RoadAccess::Type::No},
      {Segment(kFakeNumMwmId, 2, 2, false), RoadAccess::Type::Private},
  };

  map<Segment, RoadAccess::Type> const m1 = {
      {Segment(kFakeNumMwmId, 1, 1, false), RoadAccess::Type::Private},
      {Segment(kFakeNumMwmId, 2, 0, true), RoadAccess::Type::Destination},
  };

  RoadAccess roadAccessCar(kCarMask);
  roadAccessCar.SetTypes(m0);

  RoadAccess roadAccessPedestrian(kPedestrianMask);
  roadAccessPedestrian.SetTypes(m1);

  map<VehicleMask, RoadAccess> const ms = {
      {kCarMask, roadAccessCar},
      {kPedestrianMask, roadAccessPedestrian},
  };

  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    RoadAccessSerializer::Serialize(writer, ms);
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, kCarMask, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessCar, deserializedRoadAccess, ());
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, kPedestrianMask, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessPedestrian, deserializedRoadAccess, ());
  }
}
}  // namespace
