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
template <typename Obj>
void TestDeserializerFromJson(string const & jsonBuffer, string const & name, vector<Obj> const & expected)
{
  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json."));

  DeserializerFromJson deserializer(root.get());

  vector<Obj> objects;
  deserializer(objects, name.c_str());

  TEST_EQUAL(objects.size(), expected.size(), ());
  for (size_t i = 0; i < objects.size(); ++i)
    TEST(objects[i].IsEqualForTesting(expected[i]), (objects[i], expected[i]));
}

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

  vector<Stop> const expected = {
      Stop(343259523 /* id */, 1234 /* featureId */, kInvalidTransferId /* transfer id */,
           {19207936, 19207937} /* lineIds */,  {27.4970954, 64.20146835878187} /* point */),
      Stop(266680843 /* id */, 2345 /* featureId */, 5 /* transfer id */,
           {19213568, 19213569} /* lineIds */, {27.5227942, 64.25206634443111} /* point */)};

  TestDeserializerFromJson(jsonBuffer, "stops", expected);
}

UNIT_TEST(DeserializerFromJson_Edges)
{
  string const jsonBuffer = R"(
  {
  "edges": [
    {
      "finish_stop_id": 442018445,
      "line_id": 72551680,
      "shape_ids": [5, 7],
      "weight" : 234.5,
      "start_stop_id": 442018444,
      "transfer": false
    },
    {
      "finish_stop_id": 442018446,
      "line_id": 72551680,
      "shape_ids": [],
      "weight" : 345.6,
      "start_stop_id": 442018445,
      "transfer": false
    }
  ]})";

  vector<Edge> const expected = {
    Edge(442018444 /* start stop id */, 442018445 /* finish stop id */, 234.5 /* weight */,
         72551680 /* line id */,  false /* transfer */, {5, 7} /* shape ids */),
    Edge(442018445 /* start stop id */, 442018446 /* finish stop id */, 345.6 /* weight */,
         72551680 /* line id */,  false /* transfer */, {} /* shape ids */)};

  TestDeserializerFromJson(jsonBuffer, "edges", expected);
}
}  // namespace
