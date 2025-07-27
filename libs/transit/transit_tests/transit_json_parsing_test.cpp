#include "testing/testing.hpp"

#include "transit/transit_tests/transit_tools.hpp"

#include "transit/transit_graph_data.hpp"
#include "transit/transit_types.hpp"

#include "base/assert.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace routing;
using namespace routing::transit;
using namespace std;

namespace
{
template <typename Obj>
void TestDeserializerFromJson(string const & jsonBuffer, OsmIdToFeatureIdsMap const & osmIdToFeatureIds,
                              string const & name, vector<Obj> const & expected)
{
  base::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json."));

  DeserializerFromJson deserializer(root.get(), osmIdToFeatureIds);

  vector<Obj> objects;
  deserializer(objects, name.c_str());

  TEST_EQUAL(objects.size(), expected.size(), ());
  TestForEquivalence(objects, expected);
}

template <typename Obj>
void TestDeserializerFromJson(string const & jsonBuffer, string const & name, vector<Obj> const & expected)
{
  return TestDeserializerFromJson(jsonBuffer, OsmIdToFeatureIdsMap(), name, expected);
}

UNIT_TEST(DeserializerFromJson_TitleAnchors)
{
  string const jsonBuffer = R"(
  {
  "title_anchors": [
    { "min_zoom": 11, "anchor": 4 },
    { "min_zoom": 14, "anchor": 6 }
  ]})";

  vector<TitleAnchor> expected = {TitleAnchor(11 /* min zoom */, 4 /* anchor */),
                                  TitleAnchor(14 /* min zoom */, 6 /* anchor */)};
  TestDeserializerFromJson(jsonBuffer, "title_anchors", expected);
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
      "osm_id": "1234",
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
      "osm_id": "2345",
      "point": {
        "x": 27.5227942,
        "y": 64.25206634443111
      },
      "title_anchors": [
        { "min_zoom": 12, "anchor": 0 },
        { "min_zoom": 15, "anchor": 9 }]
    }
  ]})";

  vector<Stop> const expected = {
      Stop(343259523 /* id */, 1234 /* osm id */, 1 /* feature id */, kInvalidTransferId /* transfer id */,
           {19207936, 19207937} /* lineIds */, {27.4970954, 64.20146835878187} /* point */, {} /* anchors */),
      Stop(266680843 /* id */, 2345 /* osm id */, 2 /* feature id */, 5 /* transfer id */,
           {19213568, 19213569} /* line ids */, {27.5227942, 64.25206634443111} /* point */,
           {TitleAnchor(12 /* min zoom */, 0 /* anchor */), TitleAnchor(15, 9)})};

  OsmIdToFeatureIdsMap mapping;
  mapping[base::GeoObjectId(1234)] = vector<FeatureId>({1});
  mapping[base::GeoObjectId(2345)] = vector<FeatureId>({2});
  TestDeserializerFromJson(jsonBuffer, mapping, "stops", expected);
}

UNIT_TEST(DeserializerFromJson_Gates)
{
  string const jsonBuffer = R"(
  {
  "gates": [
    {
      "entrance": true,
      "exit": true,
      "osm_id": "46116860",
      "point": {
        "x": 43.8594864,
        "y": 68.33320554776377
       },
       "stop_ids": [ 442018474 ],
       "weight": 60
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "18446744073709551615",
      "point": {
        "x": 43.9290544,
        "y": 68.41120791512581
       },
       "stop_ids": [ 442018465 ],
       "weight": 60
    }
  ]})";

  vector<Gate> const expected = {
      Gate(46116860 /* osm id */, 0 /* feature id */, true /* entrance */, true /* exit */, 60 /* weight */,
           {442018474} /* stop ids */, {43.8594864, 68.33320554776377} /* point */),
      Gate(18446744073709551615ULL /* osm id */, 2 /* feature id */, true /* entrance */, true /* exit */,
           60 /* weight */, {442018465} /* stop ids */, {43.9290544, 68.41120791512581} /* point */)};

  OsmIdToFeatureIdsMap mapping;
  mapping[base::GeoObjectId(46116860)] = vector<FeatureId>({0});
  // Note. std::numeric_limits<uint64_t>::max() == 18446744073709551615
  mapping[base::GeoObjectId(18446744073709551615ULL)] = vector<FeatureId>({2});
  TestDeserializerFromJson(jsonBuffer, mapping, "gates", expected);
}

UNIT_TEST(DeserializerFromJson_Edges)
{
  string const jsonBuffer = R"(
  {
  "edges": [
    {
      "stop2_id": 442018445,
      "line_id": 72551680,
      "shape_ids": [
        {
          "stop1_id": 209186407,
          "stop2_id": 209186410
        },
        {
          "stop1_id": 209186408,
          "stop2_id": 209186411
        }
      ],
      "stop1_id": 442018444,
      "transfer": false
    },
    {
      "stop2_id": 442018446,
      "line_id": 72551680,
      "shape_ids": [],
      "weight" : 345,
      "stop1_id": 442018445,
      "transfer": false
    }
  ]})";

  vector<Edge> const expected = {
      Edge(442018444 /* stop 1 id */, 442018445 /* stop 2 id */, kInvalidWeight /* weight */, 72551680 /* line id */,
           false /* transfer */, {ShapeId(209186407, 209186410), ShapeId(209186408, 209186411)}),
      Edge(442018445 /* stop 1 id */, 442018446 /* stop 2 id */, 345 /* weight */, 72551680 /* line id */,
           false /* transfer */, {} /* shape ids */)};

  TestDeserializerFromJson(jsonBuffer, "edges", expected);
}

UNIT_TEST(DeserializerFromJson_Transfers)
{
  string const jsonBuffer = R"(
  {
  "transfers": [
    {
      "id": 922337203,
      "point": {
        "x": 27.5619844,
        "y": 64.24325959173672
      },
      "stop_ids": [
        209186416,
        277039518
      ]
    }
  ]})";

  vector<Transfer> const expected = {Transfer(922337203 /* stop id */, {27.5619844, 64.24325959173672} /* point */,
                                              {209186416, 277039518} /* stopIds */, {} /* anchors */)};

  TestDeserializerFromJson(jsonBuffer, "transfers", expected);
}

UNIT_TEST(DeserializerFromJson_Lines)
{
  string const jsonBuffer = R"(
  {
  "lines": [
    {
      "id": 19207936,
      "interval": 150,
      "network_id": 2,
      "number": "1",
      "stop_ids": [
        343262691,
        343259523,
        343252898,
        209191847,
        2947858576
      ],
      "title": "Московская линия",
      "type": "subway",
      "color": "green"
    },
    {
      "id": 19207937,
      "interval": 150,
      "network_id": 2,
      "number": "2",
      "stop_ids": [
        246659391,
        246659390,
        209191855,
        209191854,
        209191853,
        209191852,
        209191851
      ],
      "title": "Московская линия",
      "type": "subway",
      "color": "red"
    }
  ]})";

  vector<Line> const expected = {
      Line(19207936 /* line id */, "1" /* number */, "Московская линия" /* title */, "subway" /* type */,
           "green" /* color */, 2 /* network id */,
           {{343262691, 343259523, 343252898, 209191847, 2947858576}} /* stop ids */, 150 /* interval */),
      Line(19207937 /* line id */, "2" /* number */, "Московская линия" /* title */, "subway" /* type */,
           "red" /* color */, 2 /* network id */,
           {{246659391, 246659390, 209191855, 209191854, 209191853, 209191852, 209191851}} /* stop ids */,
           150 /* interval */)};

  TestDeserializerFromJson(jsonBuffer, "lines", expected);
}

UNIT_TEST(DeserializerFromJson_Shapes)
{
  string const jsonBuffer = R"(
  {
  "shapes": [
    {
      "id": {
        "stop1_id": 209186424,
        "stop2_id": 248520179
      },
      "polyline": [
        {
          "x": 27.5762295,
          "y": 64.256768574044699
        },
        {
          "x": 27.576325736220355,
          "y": 64.256879325696005
        },
        {
          "x": 27.576420780761875,
          "y": 64.256990221238539
        },
        {
          "x": 27.576514659541523,
          "y": 64.257101255242176
        }
      ]
    },
    {
      "id": {
        "stop1_id": 209191850,
        "stop2_id": 209191851
      },
      "polyline": [
        {
          "x": 27.554025800000002,
          "y": 64.250591911669844
        },
        {
          "x": 27.553906184631536,
          "y": 64.250633404586054
        }
      ]
    }
  ]})";

  vector<Shape> const expected = {
      Shape(ShapeId(209186424 /* stop 1 id */, 248520179 /* stop 2 id */),
            {m2::PointD(27.5762295, 64.256768574044699), m2::PointD(27.576325736220355, 64.256879325696005),
             m2::PointD(27.576420780761875, 64.256990221238539),
             m2::PointD(27.576514659541523, 64.257101255242176)} /* polyline */),
      Shape(ShapeId(209191850 /* stop 1 id */, 209191851 /* stop 2 id */),
            {m2::PointD(27.554025800000002, 64.250591911669844),
             m2::PointD(27.553906184631536, 64.250633404586054)} /* polyline */)};

  TestDeserializerFromJson(jsonBuffer, "shapes", expected);
}

UNIT_TEST(DeserializerFromJson_Networks)
{
  string const jsonBuffer = R"(
  {
  "networks": [
    {
      "id": 2,
      "title": "Минский метрополитен"
    }
  ]})";

  vector<Network> const expected = {Network(2 /* network id */, "Минский метрополитен" /* title */)};
  TestDeserializerFromJson(jsonBuffer, "networks", expected);
}
}  // namespace
