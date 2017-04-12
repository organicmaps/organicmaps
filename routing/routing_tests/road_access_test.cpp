#include "testing/testing.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/routing_tests/index_graph_tools.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace routing;
using namespace routing_test;
using namespace std;

namespace
{
UNIT_TEST(RoadAccess_Serialization)
{
  vector<uint32_t> privateRoads = {1, 2, 3, 100};
  RoadAccess roadAccess;
  roadAccess.SetPrivateRoads(move(privateRoads));

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
  }

  TEST_EQUAL(roadAccess, deserializedRoadAccess, ());

  {
    auto const & b = deserializedRoadAccess.GetPrivateRoads();
    TEST(is_sorted(b.begin(), b.end()), ());
  }
}
}  // namespace
