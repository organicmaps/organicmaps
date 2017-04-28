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
  // Segment is (numMwmId, featureId, segmentIdx, isForward).
  map<Segment, RoadAccess::Type> const m0 = {
      {Segment(kFakeNumMwmId, 1, 0, false), RoadAccess::Type::No},
      {Segment(kFakeNumMwmId, 2, 2, false), RoadAccess::Type::Private},
  };

  map<Segment, RoadAccess::Type> const m1 = {
      {Segment(kFakeNumMwmId, 1, 1, false), RoadAccess::Type::Private},
      {Segment(kFakeNumMwmId, 2, 0, true), RoadAccess::Type::Destination},
  };

  RoadAccess roadAccessCar;
  roadAccessCar.SetSegmentTypes(m0);

  RoadAccess roadAccessPedestrian;
  roadAccessPedestrian.SetSegmentTypes(m1);

  RoadAccessSerializer::RoadAccessByVehicleType roadAccessAllTypes;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Car)] = roadAccessCar;
  roadAccessAllTypes[static_cast<size_t>(VehicleType::Pedestrian)] = roadAccessPedestrian;

  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    RoadAccessSerializer::Serialize(writer, roadAccessAllTypes);
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, VehicleType::Car, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessCar, deserializedRoadAccess, ());
  }

  {
    RoadAccess deserializedRoadAccess;

    MemReader memReader(buf.data(), buf.size());
    ReaderSource<MemReader> src(memReader);
    RoadAccessSerializer::Deserialize(src, VehicleType::Pedestrian, deserializedRoadAccess);
    TEST_EQUAL(src.Size(), 0, ());

    TEST_EQUAL(roadAccessPedestrian, deserializedRoadAccess, ());
  }
}
}  // namespace
