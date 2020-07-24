#include "testing/testing.hpp"

#include "transit/transit_tests/transit_tools.hpp"

#include "transit/experimental/transit_data.hpp"

#include "base/assert.hpp"

#include <string>
#include <utility>
#include <vector>

namespace transit
{
namespace experimental
{
template <typename... Args>
void FillContainer(std::vector<std::string> const & lineByLineJsons, Args &&... args)
{
  for (auto const & line : lineByLineJsons)
    Read(base::Json(line.c_str()), std::forward<Args>(args)...);
}

UNIT_TEST(ReadJson_Network)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "id":4032061478,
           "title":[
             {
               "lang":"en",
               "text":"Golden Gate Transit"
             },
             {
               "lang":"sp",
               "text":"Tránsito Golden Gate"
             }
           ]
         })",
      R"({
            "id":4035419389,
            "title":[
              {
                "lang":"default",
                "text":"Caltrain"
              }
            ]
          })"};

  std::vector<Network> const networksPlan = {
      Network(4032061478 /* transitId */, Translations{{"en", "Golden Gate Transit"},
                                                       {"sp", "Tránsito Golden Gate"}} /* title */),
      Network(4035419389 /* transitId */, Translations{{"default", "Caltrain"}} /* title */)};

  std::vector<Network> networksFact;

  FillContainer(lineByLineJson, networksFact);
  TestEqual(networksFact, networksPlan);
}

UNIT_TEST(ReadJson_Route)
{
  std::vector<std::string> const lineByLineJson{
      R"({
            "id":4036206863,
            "network_id":4036206862,
            "color":"pink_dark",
            "type":"rail",
            "title":[
              {
                "lang":"en",
                "text":"Main Line"
              },
              {
                "lang":"sp",
                "text":"Línea principal"
              }
            ]
          })",
      R"({
           "id":4027700598,
           "network_id":4027700597,
           "color":"blue",
           "type":"bus",
           "title":[
             {
               "lang":"default",
               "text":"East Route"
             }
           ]
         })"};

  std::vector<Route> const routesPlan = {
      Route(4036206863 /* id */, 4036206862 /* networkId */, "rail" /* routeType */,
            Translations{{"en", "Main Line"}, {"sp", "Línea principal"}} /* title */,
            "pink_dark" /* color */),
      Route(4027700598 /* id */, 4027700597 /* networkId */, "bus" /* routeType */,
            Translations{{"default", "East Route"}} /* title */, "blue" /* color */)};

  std::vector<Route> routesFact;

  FillContainer(lineByLineJson, routesFact);
  TestEqual(routesFact, routesPlan);
}

UNIT_TEST(ReadJson_Line)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "id":4036591532,
           "route_id":4036591423,
           "shape":{
             "id":4036591460,
             "start_index":415,
             "end_index":1691
           },
           "title":[
             {
               "lang":"en",
               "text":"Downtown"
             }
           ],
           "stops_ids":[
             4036592571,
             4036592572,
             4036592573
           ],
           "service_days":"2021 Nov 25-2021 Nov 26, 2021 Dec 24-2021 Dec 25 closed",
           "intervals":[
                         {
                           "interval_s":3600,
                           "service_hours":"06:40-18:40 open"
                         }
                       ]
         })"};

  std::vector<Line> const linesPlan = {
      Line(4036591532 /* id */, 4036591423 /* routeId */,
           ShapeLink(4036591460 /* id */, 415 /* startIndex */, 1691 /* endIndex */),
           Translations{{"en", "Downtown"}} /* title */, IdList{4036592571, 4036592572, 4036592573},
           std::vector<LineInterval>{LineInterval(
               3600 /* headwayS */,
               osmoh::OpeningHours("06:40-18:40 open") /* timeIntervals */)} /* intervals */,
           osmoh::OpeningHours(
               "2021 Nov 25-2021 Nov 26, 2021 Dec 24-2021 Dec 25 closed") /* serviceDays */)};

  std::vector<Line> linesFact;

  FillContainer(lineByLineJson, linesFact);
  TestEqual(linesFact, linesPlan);
}

UNIT_TEST(ReadJson_Stop)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "id":4036592706,
           "point":{
             "x":-121.74124399999999,
             "y":41.042765953900343
           },
           "title":[
             {
               "lang":"default",
               "text":"Balfour Rd & Foothill Dr"
             }
           ],
           "timetable":[
             {
               "line_id":4036591493,
               "arrivals":"13:23-13:23 open"
             },
             {
               "line_id":4036591562,
               "arrivals":"15:23-15:23 open"
             }
           ],
           "transfer_ids":[
             4036593809,
             4036595406
           ]
         })"};

  std::vector<Stop> const stopsPlan = {
      Stop(4036592706 /* id */, kInvalidFeatureId /* featureId */, kInvalidOsmId /* osmId */,
           Translations{{"default", "Balfour Rd & Foothill Dr"}},
           TimeTable{{4036591493, osmoh::OpeningHours("13:23-13:23 open")},
                     {4036591562, osmoh::OpeningHours("15:23-15:23 open")}},
           m2::PointD(-121.74124, 41.04276), {4036593809, 4036595406} /* transferIds */)};

  std::vector<Stop> stopsFact;

  OsmIdToFeatureIdsMap dummyMapping;
  FillContainer(lineByLineJson, stopsFact, dummyMapping);
  TestEqual(stopsFact, stopsPlan);
}

UNIT_TEST(ReadJson_Gate)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "id":4034666808,
           "weights":[
             {
               "stop_id":4034666299,
               "time_to_stop":76
             },
             {
               "stop_id":4034666190,
               "time_to_stop":61
             }
           ],
           "exit":true,
           "entrance":true,
           "point":{
             "x":-121.840608,
             "y":40.19395285260439
           }
         })"};

  std::vector<Gate> const gatesPlan = {
      Gate(4034666808 /* id */, kInvalidFeatureId /* featureId */, kInvalidOsmId /* osmId */,
           true /* entrance */, true /* exit */,
           std::vector<TimeFromGateToStop>{
               TimeFromGateToStop(4034666299 /* stopId */, 76 /* timeSeconds */),
               TimeFromGateToStop(4034666190 /* stopId */, 61 /* timeSeconds */)},
           m2::PointD(-121.8406, 40.19395))};

  std::vector<Gate> gatesFact;

  OsmIdToFeatureIdsMap dummyMapping;
  FillContainer(lineByLineJson, gatesFact, dummyMapping);
  TestEqual(gatesFact, gatesPlan);
}

UNIT_TEST(ReadJson_Edge)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "line_id":4036591958,
           "stop_id_from":4036592295,
           "stop_id_to":4036592296,
           "weight":69,
           "shape":{
             "id":4036591484,
             "start_index":592,
             "end_index":609
           }
         })",
      R"({
           "stop_id_from":4030648032,
           "stop_id_to":4030648073,
           "weight":40
         }
      )"};

  std::vector<Edge> const edgesPlan = {
      Edge(4036592295 /* stop1Id */, 4036592296 /* stop2Id */, 69 /* weight */,
           4036591958 /* lineId */, false /* transfer */,
           ShapeLink(4036591484 /* shapeId */, 592 /* startIndex */, 609 /* endIndex */)),
      Edge(4030648032 /* stop1Id */, 4030648073 /* stop2Id */, 40 /* weight */,
           kInvalidTransitId /* lineId */, true /* transfer */,
           ShapeLink(kInvalidTransitId /* shapeId */, 0 /* startIndex */, 0 /* endIndex */))};

  std::vector<Edge> edgesFact;

  FillContainer(lineByLineJson, edgesFact);
  TestEqual(edgesFact, edgesPlan);
}

UNIT_TEST(ReadJson_Transfer)
{
  std::vector<std::string> const lineByLineJson{
      R"({
           "id":4029752024,
           "point":{
             "x":-122.16915443000001,
             "y":40.41578164679229
           },
           "stops_ids":[
             4029751945,
             4029752010
           ]
         }
      )"};

  std::vector<Transfer> const transfersPlan = {
      Transfer(4029752024 /* id */, m2::PointD(-122.16915, 40.41578) /* point */,
               IdList{4029751945, 4029752010} /* stopIds */)};

  std::vector<Transfer> transfersFact;

  FillContainer(lineByLineJson, transfersFact);
  TestEqual(transfersFact, transfersPlan);
}
}  // namespace experimental
}  // namespace transit
