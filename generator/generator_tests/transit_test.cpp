#include "testing/testing.hpp"

#include "generator/transit_generator.hpp"

#include "routing_common/transit_types.hpp"

#include "base/assert.hpp"

#include <string>
#include <vector>

using namespace routing;
using namespace routing::transit;
using namespace std;

namespace
{
UNIT_TEST(DeserializerFromJson_Stops)
{
  string const jsonBuffer = R"(
  {
  "stops": [{
      "id": 343259523,
      "line_ids": [
        19207936,
        19207937
      ],
      "osm_id": 1234,
      "point": {
        "x": 27.4970954,
        "y": 64.20146835878187
      },
      "title_anchors": []
    },
    {
      "id": 266680843,
      "transfer_id" : 5,
      "line_ids": [
        19213568,
        19213569
      ],
      "osm_id": 2345,
      "point": {
        "x": 27.5227942,
        "y": 64.25206634443111
      },
      "title_anchors": []
    }
  ]})";

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json."));

  DeserializerFromJson deserializer(root.get());

  vector<Stop> stops;
  deserializer(stops, "stops");

  vector<Stop> const expected = {
      Stop(343259523 /* id */, 1234 /* featureId */, kTransferIdInvalid /* transfer id */,
           {19207936, 19207937} /* lineIds */,  {27.4970954, 64.20146835878187} /* point */),
      Stop(266680843 /* id */, 2345 /* featureId */, 5 /* transfer id */,
           {19213568, 19213569} /* lineIds */, {27.5227942, 64.25206634443111} /* point */)};

  TEST_EQUAL(stops.size(), expected.size(), ());
  for (size_t i = 0; i < stops.size(); ++i)
    TEST(stops[i].IsEqualForTesting(expected[i]), (stops[i], "is not equal to", expected[i]));
}
}  // namespace
