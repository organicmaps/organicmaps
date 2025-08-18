#include "testing/testing.hpp"

#include "transit/world_feed/subway_converter.hpp"
#include "transit/world_feed/world_feed.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string const kSubwayTestsDir = "transit_subway_converter_tests";
std::string const kSubwayJsonFile = "subways.json";
std::string const kMappingFile = "mapping.txt";
std::string const kMappingEdgesFile = "mapping_edges.txt";

void WriteStringToFile(std::string const & fileName, std::string const & data)
{
  std::ofstream file;
  file.open(fileName);
  CHECK(file.is_open(), ("Could not open file", fileName));
  file << data;
}
}  // namespace

namespace transit
{
class SubwayConverterTests
{
public:
  SubwayConverterTests() : m_mwmMatcher(GetPlatform().ResourcesDir(), false /* haveBordersForWholeWorld */)
  {
    CHECK(Platform::MkDirChecked(kSubwayTestsDir), ("Could not create directory for test data:", kSubwayTestsDir));
    m_generator = transit::IdGenerator(base::JoinPath(kSubwayTestsDir, kMappingFile));
    m_generatorEdges = transit::IdGenerator(base::JoinPath(kSubwayTestsDir, kMappingEdgesFile));
  }

  ~SubwayConverterTests() { Platform::RmDirRecursively(kSubwayTestsDir); }

  void ParseEmptySubway()
  {
    std::string const emptySubway = R"({
    "networks":[],
    "lines":[],
    "edges":[],
    "gates":[],
    "shapes":[],
    "stops":[],
    "transfers":[]
    })";

    auto const & filePath = base::JoinPath(kSubwayTestsDir, kSubwayJsonFile);
    WriteStringToFile(filePath, emptySubway);
    transit::WorldFeed feed(m_generator, m_generatorEdges, m_colorPicker, m_mwmMatcher);
    transit::SubwayConverter converter(filePath, feed);
    TEST(!converter.Convert(), ());
  }

  // Subway consists of two lines with one transfer.
  // 65687110 ---------> 76638582 ---------> 61447662 ---------> 61447702  | Line 244075840
  //                                            |                 /
  //                                         61447663 ---------->/         | Line 244075841
  void ParseValidSubway()
  {
    std::string const validSubway = R"({
                                "networks":[
                                  {
                                    "id":108,
                                    "title":"Belo Horizonte"
                                  }
                                ],
                                "lines":[
                                  {
                                    "color":"orange_light",
                                    "id":244075840,
                                    "interval":150,
                                    "network_id":108,
                                    "number":"L1",
                                    "stop_ids":[
                                      65687110,
                                      76638582,
                                      61447662,
                                      61447702
                                    ],
                                    "title":"Metro L1R: Place des Martyrs - Ain Naâdja",
                                    "type":"subway"
                                  },
                                  {
                                    "color":"amber_dark",
                                    "id":244075841,
                                    "interval":320,
                                    "network_id":108,
                                    "number":"L1",
                                    "stop_ids":[
                                      61447663,
                                      61447702
                                    ],
                                    "title":"Metro L1R: Place des Martyrs - Ain Naâdja",
                                    "type":"subway"
                                  }
                                ],
                                "stops":[
                                  {
                                    "id":65687110,
                                    "line_ids":[
                                      244075840
                                    ],
                                    "osm_id":"4611686021817678228",
                                    "point":{
                                      "x":21.056,
                                      "y":61.316
                                    },
                                    "title_anchors":[]
                                  },
                                  {
                                    "id":76638582,
                                    "line_ids":[
                                      244075840
                                    ],
                                    "osm_id":"4611686021817678229",
                                    "point":{
                                      "x":21.057,
                                      "y":61.317
                                    },
                                    "title_anchors":[]
                                  },
                                  {
                                    "id":61447662,
                                    "line_ids":[
                                      244075840
                                    ],
                                    "osm_id":"4611686021817678230",
                                    "point":{
                                      "x":21.058,
                                      "y":61.319
                                    },
                                    "title_anchors":[]
                                  },
                                  {
                                    "id":61447702,
                                    "line_ids":[
                                      244075840,
                                      244075841
                                    ],
                                    "osm_id":"4611686021817678231",
                                    "point":{
                                      "x":21.059,
                                      "y":61.321
                                    },
                                    "title_anchors":[]
                                  },
                                  {
                                    "id":61447663,
                                    "line_ids":[
                                      244075841
                                    ],
                                    "osm_id":"4611686021817678231",
                                    "point":{
                                      "x":21.068,
                                      "y":61.399
                                    },
                                    "title_anchors":[]
                                  }
                                ],
                                "edges":[
                                  {
                                    "stop1_id":61447662,
                                    "stop2_id":61447663,
                                    "transfer":true,
                                    "weight":225
                                  },
                                  {
                                    "line_id":244075840,
                                    "shape_ids":[
                                      {
                                        "stop1_id":65687110,
                                        "stop2_id":76638582
                                      }
                                    ],
                                    "stop1_id":65687110,
                                    "stop2_id":76638582,
                                    "transfer":false,
                                    "weight":33
                                  },
                                  {
                                    "line_id":244075840,
                                    "shape_ids":[
                                      {
                                        "stop1_id":76638582,
                                        "stop2_id":61447662
                                      }
                                    ],
                                    "stop1_id":76638582,
                                    "stop2_id":61447662,
                                    "transfer":false,
                                    "weight":46
                                  },
                                  {
                                    "line_id":244075840,
                                    "shape_ids":[
                                      {
                                        "stop1_id":61447662,
                                        "stop2_id":61447702
                                      }
                                    ],
                                    "stop1_id":61447662,
                                    "stop2_id":61447702,
                                    "transfer":false,
                                    "weight":32
                                  },
                                  {
                                    "line_id":244075841,
                                    "shape_ids":[
                                      {
                                        "stop1_id":61447662,
                                        "stop2_id":61447702
                                      }
                                    ],
                                    "stop1_id":61447663,
                                    "stop2_id":61447702,
                                    "transfer":false,
                                    "weight":40
                                  }
                                ],
                                "gates":[
                                  {
                                    "entrance":true,
                                    "exit":true,
                                    "osm_id":"4611686018706540014",
                                    "point":{
                                      "x":21.055938,
                                      "y":61.31740794838219
                                    },
                                    "stop_ids":[
                                      76638582
                                    ],
                                    "weight":129
                                  }
                                ],
                                "shapes":[
                                  {
                                    "id":{
                                      "stop1_id":65687110,
                                      "stop2_id":76638582
                                    },
                                    "polyline":[
                                      {
                                        "x":3.062,
                                        "y":39.616
                                      },
                                      {
                                        "x":3.063,
                                        "y":39.61
                                      }
                                    ]
                                  },
                                  {
                                    "id":{
                                      "stop1_id":76638582,
                                      "stop2_id":61447662
                                    },
                                    "polyline":[
                                      {
                                        "x":3.063,
                                        "y":39.61
                                      },
                                      {
                                        "x":3.0624,
                                        "y":39.614
                                      }
                                    ]
                                  },
                                  {
                                    "id":{
                                      "stop1_id":61447662,
                                      "stop2_id":61447702
                                    },
                                    "polyline":[
                                      {
                                        "x":3.062,
                                        "y":39.616
                                      },
                                      {
                                        "x":3.065,
                                        "y":39.61
                                      }
                                    ]
                                  },
                                  {
                                    "id":{
                                      "stop1_id":61447663,
                                      "stop2_id":61447702
                                    },
                                    "polyline":[
                                      {
                                        "x":3.068,
                                        "y":39.64
                                      },
                                      {
                                        "x":3.063,
                                        "y":39.674
                                      }
                                    ]
                                  }
                                ],
                                "transfers":[
                                  {
                                    "id":4611686018489133646,
                                    "point":{
                                      "x":-3.6761538,
                                      "y":44.26760612521531
                                    },
                                    "stop_ids":[
                                      61447662,
                                      61447663
                                    ],
                                    "title_anchors":[

                                    ]
                                  }
                                ]
                              })";

    auto const & filePath = base::JoinPath(kSubwayTestsDir, kSubwayJsonFile);
    WriteStringToFile(filePath, validSubway);
    transit::WorldFeed feed(m_generator, m_generatorEdges, m_colorPicker, m_mwmMatcher);
    transit::SubwayConverter converter(filePath, feed);

    // We check that the conversion between old and new formats is successful.
    TEST(converter.Convert(), ());

    // We check that transit entities are converted correctly.
    TEST_EQUAL(feed.m_networks.m_data.size(), 1, ());
    TEST_EQUAL(feed.m_routes.m_data.size(), 1, ());
    TEST_EQUAL(feed.m_lines.m_data.size(), 2, ());
    TEST_EQUAL(feed.m_stops.m_data.size(), 5, ());
    TEST_EQUAL(feed.m_edges.m_data.size(), 4, ());
    TEST_EQUAL(feed.m_edgesTransfers.m_data.size(), 1, ());
    TEST_EQUAL(feed.m_transfers.m_data.size(), 0, ());
    TEST_EQUAL(feed.m_gates.m_data.size(), 1, ());

    // Two initial shapes must be merged into one.
    TEST_EQUAL(feed.m_shapes.m_data.size(), 1, ());

    // Shape does not contain duplicate points (we have 5 points instead of 6).
    TEST_EQUAL(feed.m_shapes.m_data.begin()->second.m_points.size(), 5, ());

    // We check main relations consistency.
    auto const networkIt = feed.m_networks.m_data.begin();
    auto const routeIt = feed.m_routes.m_data.begin();

    TEST_EQUAL(routeIt->second.m_networkId, networkIt->first, ());

    for (auto const & [lineId, lineData] : feed.m_lines.m_data)
    {
      TEST_EQUAL(lineData.m_routeId, routeIt->first, ());

      for (auto const stopId : lineData.m_stopIds)
      {
        auto const stopIt = feed.m_stops.m_data.find(stopId);
        TEST(stopIt != feed.m_stops.m_data.end(), (stopId));
      }
    }
  }

private:
  transit::IdGenerator m_generator;
  transit::IdGenerator m_generatorEdges;
  transit::ColorPicker m_colorPicker;
  feature::CountriesFilesAffiliation m_mwmMatcher;
};

UNIT_CLASS_TEST(SubwayConverterTests, SubwayConverter_ParseInvalidJson)
{
  ParseEmptySubway();
}

UNIT_CLASS_TEST(SubwayConverterTests, SubwayConverter_ParseValidJson)
{
  ParseValidSubway();
}
}  // namespace transit
