#include "testing/testing.hpp"

#include "transit/transit_tests/transit_tools.hpp"

#include "transit/transit_graph_data.hpp"
#include "transit/transit_types.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace routing;
using namespace routing::transit;
using namespace std;

namespace
{
struct Graph
{
  vector<Stop> m_stops;
  vector<Gate> m_gates;
  vector<Edge> m_edges;
  vector<Transfer> m_transfers;
  vector<Line> m_lines;
  vector<Shape> m_shapes;
  vector<Network> m_networks;
};

unique_ptr<GraphData> CreateGraphFromJson()
{
  string const jsonBuffer = R"(
  {
  "stops": [{
      "id": 0,
      "line_ids": [1],
      "osm_id": "100",
      "point": { "x": -2.0, "y": 1.0 },
      "title_anchors": []
    },
    {
      "id": 1,
      "line_ids": [1],
      "osm_id": "101",
      "point": { "x": 0.0, "y": 1.0 },
      "title_anchors": []
    },
    {
      "id": 2,
      "line_ids": [1],
      "osm_id": "102",
      "point": { "x": 2.0, "y": 1.0 },
      "title_anchors": []
    },
    {
      "id": 3,
      "line_ids": [1],
      "osm_id": "103",
      "point": { "x": 4.0, "y": 1.0 },
      "title_anchors": []
    },
    {
      "id": 4,
      "line_ids": [1],
      "osm_id": "104",
      "point": { "x": 5.0, "y": 1.0 },
      "title_anchors": []
    },
    {
      "id": 5,
      "line_ids": [2],
      "osm_id": "105",
      "point": { "x": -1.0, "y": -1.0 },
      "title_anchors": []
    },
    {
      "id": 6,
      "line_ids": [2],
      "osm_id": "106",
      "point": { "x": 1.0, "y": -1.0 },
      "title_anchors": []
    }
  ],
  "lines": [
    {
      "id": 1,
      "interval": 150,
      "network_id": 2,
      "number": "1",
      "stop_ids": [ 0, 1, 2, 3, 4 ],
      "title": "Московская линия",
      "type": "subway",
      "color": "green"
    },
    {
      "id": 2,
      "interval": 150,
      "network_id": 2,
      "number": "2",
      "stop_ids": [ 5, 6 ],
      "title": "Варшавская линия",
      "type": "subway",
      "color": "red"
    }
  ],
  "transfers": [ ],
  "networks": [
    {
      "id": 2,
      "title": "Минский метрополитен"
    }
  ],
  "edges": [
    {
      "stop1_id": 0,
      "stop2_id": 1,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 0, "stop2_id": 1 }],
      "transfer": false,
      "weight" : 20
    },
    {
      "stop1_id": 1,
      "stop2_id": 2,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 1, "stop2_id": 2 }],
      "transfer": false,
      "weight" : 20
    },
    {
      "stop1_id": 2,
      "stop2_id": 3,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 2, "stop2_id": 3 }],
      "transfer": false,
      "weight" : 20
    },
    {
      "stop1_id": 3,
      "stop2_id": 4,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 3, "stop2_id": 4 }],
      "transfer": false,
      "weight" : 10
    },
    {
      "stop1_id": 3,
      "stop2_id": 4,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 3, "stop2_id": 4 }],
      "transfer": false,
      "weight" : 20
    },
    {
      "stop1_id": 3,
      "stop2_id": 4,
      "line_id": 1,
      "shape_ids": [ { "stop1_id": 3, "stop2_id": 4 }],
      "transfer": false,
      "weight" : 20
    },
    {
      "stop1_id": 5,
      "stop2_id": 6,
      "line_id": 2,
      "shape_ids": [ { "stop1_id": 5, "stop2_id": 6 }],
      "transfer": false,
      "weight" : 20
    }
  ],
  "shapes": [
    {
      "id": { "stop1_id": 0, "stop2_id": 1 },
      "polyline": [
        { "x": -2.0, "y": 1.0 },
        { "x": 0.0, "y": 1.0 }
      ]
    },
    {
      "id": { "stop1_id": 1, "stop2_id": 2 },
      "polyline": [
        { "x": 0.0, "y": 1.0 },
        { "x": 2.0, "y": 1.0 }
      ]
    },
    {
      "id": { "stop1_id": 2, "stop2_id": 3 },
      "polyline": [
        { "x": 2.0, "y": 1.0 },
        { "x": 4.0, "y": 1.0 }
      ]
    },
    {
      "id": { "stop1_id": 3, "stop2_id": 4 },
      "polyline": [
        { "x": 4.0, "y": 1.0 },
        { "x": 5.0, "y": 1.0 }
      ]
    },
    {
      "id": { "stop1_id": 5, "stop2_id": 6 },
      "polyline": [
        { "x": -1.0, "y": -1.0 },
        { "x": 1.0, "y": -1.0 }
      ]
    }
  ],
  "gates": [
    {
      "entrance": true,
      "exit": true,
      "osm_id": "100",
      "point": { "x": -2.0, "y": 1.0 },
      "stop_ids": [ 0 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "101",
      "point": { "x": 0.0, "y": 1.0 },
      "stop_ids": [ 1 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "102",
      "point": { "x": 2.0, "y": 1.0 },
      "stop_ids": [ 2 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "103",
      "point": { "x": 4.0, "y": 1.0 },
      "stop_ids": [ 3 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "104",
      "point": { "x": 5.0, "y": 1.0 },
      "stop_ids": [ 4 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "105",
      "point": { "x": -1.0, "y": -1.0 },
      "stop_ids": [ 5 ],
      "weight": 0
    },
    {
      "entrance": true,
      "exit": true,
      "osm_id": "106",
      "point": { "x": 1.0, "y": -1.0 },
      "stop_ids": [ 6 ],
      "weight": 0
    }
  ]})";

  auto graph = make_unique<GraphData>();

  OsmIdToFeatureIdsMap mapping;
  mapping[base::GeoObjectId(100)] = vector<FeatureId>({10});
  mapping[base::GeoObjectId(101)] = vector<FeatureId>({11});
  mapping[base::GeoObjectId(102)] = vector<FeatureId>({12});
  mapping[base::GeoObjectId(103)] = vector<FeatureId>({13});
  mapping[base::GeoObjectId(104)] = vector<FeatureId>({14});
  mapping[base::GeoObjectId(105)] = vector<FeatureId>({15});
  mapping[base::GeoObjectId(106)] = vector<FeatureId>({16});

  base::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json."));
  graph->DeserializeFromJson(root, mapping);
  return graph;
}

unique_ptr<Graph> MakeFullGraph()
{
  auto graph = make_unique<Graph>();
  graph->m_stops = {{0 /* stop id */,
                     100 /* osm id */,
                     10 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(-2.0, 1.0),
                     {}},
                    {1 /* stop id */,
                     101 /* osm id */,
                     11 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(0.0, 1.0),
                     {}},
                    {2 /* stop id */,
                     102 /* osm id */,
                     12 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(2.0, 1.0),
                     {}},
                    {3 /* stop id */,
                     103 /* osm id */,
                     13 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(4.0, 1.0),
                     {}},
                    {4 /* stop id */,
                     104 /* osm id */,
                     14 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(5.0, 1.0),
                     {}},
                    {5 /* stop id */,
                     105 /* osm id */,
                     15 /* feature id */,
                     kInvalidTransferId,
                     {2} /* line ids */,
                     m2::PointD(-1.0, -1.0),
                     {}},
                    {6 /* stop id */,
                     106 /* osm id */,
                     16 /* feature id */,
                     kInvalidTransferId,
                     {2} /* line ids */,
                     m2::PointD(1.0, -1.0),
                     {}}};

  graph->m_gates = {
      {100 /* osm id */,
       10 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {0} /* stop ids */,
       m2::PointD(-2.0, 1.0)},
      {101 /* osm id */,
       11 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {1} /* stop ids */,
       m2::PointD(0.0, 1.0)},
      {102 /* osm id */,
       12 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {2} /* stop ids */,
       m2::PointD(2.0, 1.0)},
      {103 /* osm id */,
       13 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {3} /* stop ids */,
       m2::PointD(4.0, 1.0)},
      {104 /* osm id */,
       14 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {4} /* stop ids */,
       m2::PointD(5.0, 1.0)},
      {105 /* osm id */,
       15 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {5} /* stop ids */,
       m2::PointD(-1.0, -1.0)},
      {106 /* osm id */,
       16 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {6} /* stop ids */,
       m2::PointD(1.0, -1.0)},
  };

  graph->m_edges = {{0 /* stop 1 id */,
                     1 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{0, 1}} /* shape ids */},
                    {1 /* stop 1 id */,
                     2 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{1, 2}} /* shape ids */},
                    {2 /* stop 1 id */,
                     3 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{2, 3}} /* shape ids */},
                    {3 /* stop 1 id */,
                     4 /* stop 2 id */,
                     10 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{3, 4}} /* shape ids */},
                    {5 /* stop 1 id */,
                     6 /* stop 2 id */,
                     20 /* weight */,
                     2 /* line id */,
                     false /* transfer */,
                     {{5, 6}} /* shape ids */}};

  // |graph.m_transfers| should be empty.

  graph->m_lines = {{1 /* line id */,
                     "1" /* number */,
                     "Московская линия" /* title */,
                     "subway" /* type */,
                     "green",
                     2 /* network id */,
                     {{0, 1, 2, 3, 4}} /* stop id */,
                     150 /* interval */},
                    {2 /* line id */,
                     "2" /* number */,
                     "Варшавская линия" /* title */,
                     "subway" /* type */,
                     "red",
                     2 /* network id */,
                     {{5, 6}} /* stop id */,
                     150 /* interval */}};

  graph->m_shapes = {{{0, 1} /* shape id */, {{-2.0, 1.0}, {0.0, 1.0}} /* polyline */},
                     {{1, 2} /* shape id */, {{0.0, 1.0}, {2.0, 1.0}} /* polyline */},
                     {{2, 3} /* shape id */, {{2.0, 1.0}, {4.0, 1.0}} /* polyline */},
                     {{3, 4} /* shape id */, {{4.0, 1.0}, {5.0, 1.0}} /* polyline */},
                     {{5, 6} /* shape id */, {{-1.0, -1.0}, {1.0, -1.0}} /* polyline */}};

  graph->m_networks = {{2 /* network id */, "Минский метрополитен" /* title */}};

  return graph;
}

unique_ptr<Graph> MakeOneLineGraph()
{
  auto graph = make_unique<Graph>();

  graph->m_stops = {{0 /* stop id */,
                     100 /* osm id */,
                     10 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(-2.0, 1.0),
                     {}},
                    {1 /* stop id */,
                     101 /* osm id */,
                     11 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(0.0, 1.0),
                     {}},
                    {2 /* stop id */,
                     102 /* osm id */,
                     12 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(2.0, 1.0),
                     {}},
                    {3 /* stop id */,
                     103 /* osm id */,
                     13 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(4.0, 1.0),
                     {}}};

  graph->m_gates = {{100 /* osm id */,
                     10 /* feature id */,
                     true /* entrance */,
                     true /* exit */,
                     0 /* weight */,
                     {0} /* stop ids */,
                     m2::PointD(-2.0, 1.0)},
                    {101 /* osm id */,
                     11 /* feature id */,
                     true /* entrance */,
                     true /* exit */,
                     0 /* weight */,
                     {1} /* stop ids */,
                     m2::PointD(0.0, 1.0)},
                    {102 /* osm id */,
                     12 /* feature id */,
                     true /* entrance */,
                     true /* exit */,
                     0 /* weight */,
                     {2} /* stop ids */,
                     m2::PointD(2.0, 1.0)},
                    {103 /* osm id */,
                     13 /* feature id */,
                     true /* entrance */,
                     true /* exit */,
                     0 /* weight */,
                     {3} /* stop ids */,
                     m2::PointD(4.0, 1.0)}};

  graph->m_edges = {{0 /* stop 1 id */,
                     1 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{0, 1}} /* shape ids */},
                    {1 /* stop 1 id */,
                     2 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{1, 2}} /* shape ids */},
                    {2 /* stop 1 id */,
                     3 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{2, 3}} /* shape ids */}};

  // |graph.m_transfers| should be empty.

  graph->m_lines = {{1 /* line id */,
                     "1" /* number */,
                     "Московская линия" /* title */,
                     "subway" /* type */,
                     "green",
                     2 /* network id */,
                     {{0, 1, 2, 3}} /* stop id */,
                     150 /* interval */}};

  graph->m_shapes = {{{0, 1} /* shape id */, {{-2.0, 1.0}, {0.0, 1.0}} /* polyline */},
                     {{1, 2} /* shape id */, {{0.0, 1.0}, {2.0, 1.0}} /* polyline */},
                     {{2, 3} /* shape id */, {{2.0, 1.0}, {4.0, 1.0}} /* polyline */}};

  graph->m_networks = {{2 /* network id */, "Минский метрополитен" /* title */}};

  return graph;
}

unique_ptr<Graph> MakeTwoLinesGraph()
{
  auto graph = make_unique<Graph>();

  graph->m_stops = {{1 /* stop id */,
                     101 /* osm id */,
                     11 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(0.0, 1.0),
                     {}},
                    {2 /* stop id */,
                     102 /* osm id */,
                     12 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(2.0, 1.0),
                     {}},
                    {3 /* stop id */,
                     103 /* osm id */,
                     13 /* feature id */,
                     kInvalidTransferId,
                     {1} /* line ids */,
                     m2::PointD(4.0, 1.0),
                     {}},
                    {5 /* stop id */,
                     105 /* osm id */,
                     15 /* feature id */,
                     kInvalidTransferId,
                     {2} /* line ids */,
                     m2::PointD(-1.0, -1.0),
                     {}},
                    {6 /* stop id */,
                     106 /* osm id */,
                     16 /* feature id */,
                     kInvalidTransferId,
                     {2} /* line ids */,
                     m2::PointD(1.0, -1.0),
                     {}}};

  graph->m_gates = {
      {101 /* osm id */,
       11 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {1} /* stop ids */,
       m2::PointD(0.0, 1.0)},
      {102 /* osm id */,
       12 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {2} /* stop ids */,
       m2::PointD(2.0, 1.0)},
      {103 /* osm id */,
       13 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {3} /* stop ids */,
       m2::PointD(4.0, 1.0)},
      {105 /* osm id */,
       15 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {5} /* stop ids */,
       m2::PointD(-1.0, -1.0)},
      {106 /* osm id */,
       16 /* feature id */,
       true /* entrance */,
       true /* exit */,
       0 /* weight */,
       {6} /* stop ids */,
       m2::PointD(1.0, -1.0)},
  };

  graph->m_edges = {{1 /* stop 1 id */,
                     2 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{1, 2}} /* shape ids */},
                    {2 /* stop 1 id */,
                     3 /* stop 2 id */,
                     20 /* weight */,
                     1 /* line id */,
                     false /* transfer */,
                     {{2, 3}} /* shape ids */},
                    {5 /* stop 1 id */,
                     6 /* stop 2 id */,
                     20 /* weight */,
                     2 /* line id */,
                     false /* transfer */,
                     {{5, 6}} /* shape ids */}};

  // |graph.m_transfers| should be empty.

  graph->m_lines = {{1 /* line id */,
                     "1" /* number */,
                     "Московская линия" /* title */,
                     "subway" /* type */,
                     "green",
                     2 /* network id */,
                     {{1, 2, 3}} /* stop id */,
                     150 /* interval */},
                    {2 /* line id */,
                     "2" /* number */,
                     "Варшавская линия" /* title */,
                     "subway" /* type */,
                     "red",
                     2 /* network id */,
                     {{5, 6}} /* stop id */,
                     150 /* interval */}};

  graph->m_shapes = {{{1, 2} /* shape id */, {{0.0, 1.0}, {2.0, 1.0}} /* polyline */},
                     {{2, 3} /* shape id */, {{2.0, 1.0}, {4.0, 1.0}} /* polyline */},
                     {{5, 6} /* shape id */, {{-1.0, -1.0}, {1.0, -1.0}} /* polyline */}};

  graph->m_networks = {{2 /* network id */, "Минский метрополитен" /* title */}};

  return graph;
}

void TestGraph(GraphData const & actualGraph, Graph const & expectedGraph)
{
  TestForEquivalence(actualGraph.GetStops(), expectedGraph.m_stops);
  TestForEquivalence(actualGraph.GetGates(), expectedGraph.m_gates);
  TestForEquivalence(actualGraph.GetEdges(), expectedGraph.m_edges);
  TestForEquivalence(actualGraph.GetTransfers(), expectedGraph.m_transfers);
  TestForEquivalence(actualGraph.GetLines(), expectedGraph.m_lines);
  TestForEquivalence(actualGraph.GetShapes(), expectedGraph.m_shapes);
  TestForEquivalence(actualGraph.GetNetworks(), expectedGraph.m_networks);
}

void SerializeAndDeserializeGraph(GraphData & src, GraphData & dst)
{
  vector<uint8_t> buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  src.Serialize(writer);

  MemReader reader(buffer.data(), buffer.size());
  dst.DeserializeAll(reader);
  dst.CheckValidSortedUnique();
}

//                       ^
//                       |
//                       * 2                     _______
//                                               |     |
//           s0----------s1----------s2----------s3----s4 Line 1
//                                               |_____|
//     *     *     *     *     *     *     *     *     *   -->
//    -3    -2    -1     0     1     2     3     4     5
//                 s5----------s6 Line 2
//
UNIT_TEST(ClipGraph_SmokeTest)
{
  auto graph = CreateGraphFromJson();
  graph->Sort();

  auto expectedGraph = MakeFullGraph();
  TestGraph(*graph, *expectedGraph);

  GraphData readedGraph;
  SerializeAndDeserializeGraph(*graph, readedGraph);
  TestGraph(*graph, *expectedGraph);
}

//                       ^
//                       |
//                       * 4
//
//     ------------------* 3-----------Border
//     |                                   |
//     |                 * 2               |     _______
//     |                                   |     |     |
//     |     s0----------s1----------s2----|-----s3----s4 Line 1
//     |                                   |     |_____|
//     *-----*-----*-----*-----*-----*-----*     *     *   -->
//    -3    -2    -1     0     1     2     3     4     5
//                 s5----------s6 Line 2
//
UNIT_TEST(ClipGraph_OneLineTest)
{
  auto graph = CreateGraphFromJson();
  vector<m2::PointD> points = {{3.0, 3.0}, {3.0, 0.0}, {-3.0, 0.0}, {-3.0, 3.0}, {3.0, 3.0}};
  graph->ClipGraph({m2::RegionD(points)});
  auto expectedGraph = MakeOneLineGraph();
  TestGraph(*graph, *expectedGraph);

  GraphData readedGraph;
  SerializeAndDeserializeGraph(*graph, readedGraph);
  TestGraph(*graph, *expectedGraph);
}

//                       ^
//                       |
//                       * 3
//
//                       * 2----------Border     _______
//                          |           |        |     |
//           s0----------s1-|--------s2-|--------s3----s4 Line 1
//                          |           |        |_____|
//     *     *     *     *  |  *     *  |  *     *     *   -->
//    -3    -2    -1     0  |  1     2  |  3     4     5
//         Line 2  s5-------|--s6       |
//                          |           |
//                   -2  *  -------------
//
UNIT_TEST(ClipGraph_TwoLinesTest)
{
  auto graph = CreateGraphFromJson();
  vector<m2::PointD> points = {{2.5, 2.0}, {2.5, -2.0}, {0.5, -2.0}, {0.5, 2.0}, {2.5, 2.0}};
  graph->ClipGraph({m2::RegionD(points)});

  auto expectedGraph = MakeTwoLinesGraph();
  TestGraph(*graph, *expectedGraph);

  GraphData readedGraph;
  SerializeAndDeserializeGraph(*graph, readedGraph);
  TestGraph(*graph, *expectedGraph);
}
}  // namespace
