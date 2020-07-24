#include "transit/world_feed/subway_converter.hpp"

#include "generator/transit_generator.hpp"

#include "routing/fake_feature_ids.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <iterator>

#include "3party/jansson/myjansson.hpp"
#include "3party/opening_hours/opening_hours.hpp"

namespace
{
// Returns the index of the |point| in the |shape| polyline.
size_t FindPointIndex(std::vector<m2::PointD> const & shape, m2::PointD const & point)
{
  static double constexpr eps = 1e-6;
  auto it = std::find_if(shape.begin(), shape.end(), [&point](m2::PointD const & p) {
    return base::AlmostEqualAbs(p, point, eps);
  });

  CHECK(it != shape.end(), (point));

  return std::distance(shape.begin(), it);
}

// Returns polyline found by pair of stop ids.
std::vector<m2::PointD> GetShapeByStops(std::vector<routing::transit::Shape> const & shapes,
                                        routing::transit::StopId stopId1,
                                        routing::transit::StopId stopId2)
{
  auto const itShape = std::find_if(
      shapes.begin(), shapes.end(), [stopId1, stopId2](routing::transit::Shape const & shape) {
        return shape.GetId().GetStop1Id() == stopId1 && shape.GetId().GetStop2Id() == stopId2;
      });

  CHECK(itShape != shapes.end(), (stopId1, stopId2));

  return itShape->GetPolyline();
}
}  // namespace

namespace transit
{
std::string const kHashPrefix = "mapsme_transit";
std::string const kDefaultLang = "default";
std::string const kSubwayRouteType = "subway";
std::string const kDefaultHours = "24/7";

SubwayConverter::SubwayConverter(std::string const & subwayJson, WorldFeed & feed)
  : m_subwayJson(subwayJson), m_feed(feed)
{
}

bool SubwayConverter::Convert()
{
  routing::transit::OsmIdToFeatureIdsMap emptyMapping;
  routing::transit::DeserializeFromJson(emptyMapping, m_subwayJson, m_graphData);

  if (!ConvertNetworks())
    return false;

  if (!SplitEdges())
    return false;

  if (!ConvertLinesBasedData())
    return false;

  m_feed.ModifyLinesAndShapes();

  if (!ConvertStops())
    return false;

  if (!ConvertTransfers())
    return false;

  // In contrast to the GTFS gates OSM gates for subways shouldn't be empty.
  if (!ConvertGates())
    return false;

  return ConvertEdges();
}

bool SubwayConverter::ConvertNetworks()
{
  auto const & networksSubway = m_graphData.GetNetworks();
  m_feed.m_networks.m_data.reserve(networksSubway.size());

  for (auto const & networkSubway : networksSubway)
  {
    // Subway network id is city id index approximately in interval (0, 400).
    TransitId const networkId = networkSubway.GetId();
    CHECK(!routing::FakeFeatureIds::IsTransitFeature(networkId), (networkId));

    Translations const title{{kDefaultLang, networkSubway.GetTitle()}};
    m_feed.m_networks.m_data.emplace(networkId, title);
  }

  LOG(LINFO,
      ("Converted", m_feed.m_networks.m_data.size(), "networks from subways to public transport."));

  return !m_feed.m_networks.m_data.empty();
}

bool SubwayConverter::SplitEdges()
{
  for (auto const & edgeSubway : m_graphData.GetEdges())
  {
    if (edgeSubway.GetTransfer())
      m_edgesTransferSubway.emplace_back(edgeSubway);
    else
      m_edgesSubway.emplace_back(edgeSubway);
  }

  return !m_edgesSubway.empty() && !m_edgesTransferSubway.empty();
}

std::pair<TransitId, RouteData> SubwayConverter::MakeRoute(
    routing::transit::Line const & lineSubway)
{
  std::string const & routeTitle = lineSubway.GetNumber();
  std::string const routeHash =
      BuildHash(kHashPrefix, std::to_string(lineSubway.GetNetworkId()), routeTitle);
  TransitId const routeId = m_feed.m_idGenerator.MakeId(routeHash);

  RouteData routeData;
  routeData.m_title = {{kDefaultLang, routeTitle}};
  routeData.m_routeType = kSubwayRouteType;
  routeData.m_networkId = lineSubway.GetNetworkId();
  routeData.m_color = lineSubway.GetColor();

  return {routeId, routeData};
}

std::pair<TransitId, GateData> SubwayConverter::MakeGate(routing::transit::Gate const & gateSubway)
{
  // This id is used only for storing gates in gtfs_converter tool. It is not saved to json.
  TransitId const gateId =
      m_feed.m_idGenerator.MakeId(BuildHash(kHashPrefix, std::to_string(gateSubway.GetOsmId())));
  GateData gateData;

  gateData.m_isEntrance = gateSubway.GetEntrance();
  gateData.m_isExit = gateSubway.GetExit();
  gateData.m_point = gateSubway.GetPoint();
  gateData.m_osmId = gateSubway.GetOsmId();

  for (auto stopIdSubway : gateSubway.GetStopIds())
  {
    gateData.m_weights.emplace_back(TimeFromGateToStop(m_stopIdMapping[stopIdSubway] /* stopId */,
                                                       gateSubway.GetWeight() /* timeSeconds */));
  }

  return {gateId, gateData};
}

std::pair<TransitId, TransferData> SubwayConverter::MakeTransfer(
    routing::transit::Transfer const & transferSubway)
{
  TransitId const transferId =
      m_feed.m_idGenerator.MakeId(BuildHash(kHashPrefix, std::to_string(transferSubway.GetId())));

  TransferData transferData;
  transferData.m_point = transferSubway.GetPoint();

  for (auto stopIdSubway : transferSubway.GetStopIds())
    transferData.m_stopsIds.emplace_back(m_stopIdMapping[stopIdSubway]);

  return {transferId, transferData};
}

std::pair<TransitId, LineData> SubwayConverter::MakeLine(routing::transit::Line const & lineSubway,
                                                         TransitId routeId)
{
  TransitId const lineId = lineSubway.GetId();
  CHECK(!routing::FakeFeatureIds::IsTransitFeature(lineId), (lineId));

  LineData lineData;
  lineData.m_routeId = routeId;
  lineData.m_title = {{kDefaultLang, lineSubway.GetTitle()}};
  lineData.m_intervals = {LineInterval(lineSubway.GetInterval() /* headwayS */,
                                       osmoh::OpeningHours(kDefaultHours) /* timeIntervals */)};
  lineData.m_serviceDays = osmoh::OpeningHours(kDefaultHours);

  return {lineId, lineData};
}

std::pair<EdgeId, EdgeData> SubwayConverter::MakeEdge(routing::transit::Edge const & edgeSubway)
{
  auto const lineId = edgeSubway.GetLineId();
  EdgeId const edgeId(m_stopIdMapping[edgeSubway.GetStop1Id()],
                      m_stopIdMapping[edgeSubway.GetStop2Id()], lineId);
  EdgeData edgeData;
  edgeData.m_weight = edgeSubway.GetWeight();

  auto const it = m_edgesOnShapes.find(edgeId);
  CHECK(it != m_edgesOnShapes.end(), (lineId));

  m2::PointD const pointStart = it->second.first;
  m2::PointD const pointEnd = it->second.second;

  edgeData.m_shapeLink.m_shapeId = m_feed.m_lines.m_data[lineId].m_shapeLink.m_shapeId;

  auto itShape = m_feed.m_shapes.m_data.find(edgeData.m_shapeLink.m_shapeId);
  CHECK(itShape != m_feed.m_shapes.m_data.end(), ("Shape does not exist."));

  auto const & shapePoints = itShape->second.m_points;
  CHECK(!shapePoints.empty(), ("Shape is empty."));

  edgeData.m_shapeLink.m_startIndex = FindPointIndex(shapePoints, pointStart);
  edgeData.m_shapeLink.m_endIndex = FindPointIndex(shapePoints, pointEnd);

  return {edgeId, edgeData};
}

std::pair<EdgeTransferId, size_t> SubwayConverter::MakeEdgeTransfer(
    routing::transit::Edge const & edgeSubway)
{
  EdgeTransferId const edgeTransferId(m_stopIdMapping[edgeSubway.GetStop1Id()] /* fromStopId */,
                                      m_stopIdMapping[edgeSubway.GetStop2Id()] /* toStopId */);
  return {edgeTransferId, edgeSubway.GetWeight()};
}

std::pair<TransitId, StopData> SubwayConverter::MakeStop(routing::transit::Stop const & stopSubway)
{
  TransitId const stopId = m_stopIdMapping[stopSubway.GetId()];

  StopData stopData;
  stopData.m_point = stopSubway.GetPoint();
  stopData.m_osmId = stopSubway.GetOsmId();

  return {stopId, stopData};
}

bool SubwayConverter::ConvertLinesBasedData()
{
  auto const & linesSubway = m_graphData.GetLines();
  m_feed.m_lines.m_data.reserve(linesSubway.size());
  m_feed.m_shapes.m_data.reserve(linesSubway.size());

  auto const & shapesSubway = m_graphData.GetShapes();

  for (auto const & lineSubway : linesSubway)
  {
    auto const [routeId, routeData] = MakeRoute(lineSubway);
    m_feed.m_routes.m_data.emplace(routeId, routeData);

    auto [lineId, lineData] = MakeLine(lineSubway, routeId);

    TransitId const shapeId = m_feed.m_idGenerator.MakeId(
        BuildHash(kHashPrefix, std::string("shape"), std::to_string(lineId)));
    lineData.m_shapeId = shapeId;

    ShapeData shapeData;
    shapeData.m_lineIds.insert(lineId);

    CHECK_EQUAL(lineSubway.GetStopIds().size(), 1, ("Line shouldn't be split into ranges."));

    auto const & rangeSubway = lineSubway.GetStopIds().front();
    CHECK_GREATER(rangeSubway.size(), 1, ("Range must include at least two stops."));

    for (size_t i = 0; i < rangeSubway.size(); ++i)
    {
      auto const stopIdSubway = rangeSubway[i];
      std::string const stopHash = BuildHash(kHashPrefix, std::to_string(stopIdSubway));
      TransitId const stopId = m_feed.m_idGenerator.MakeId(stopHash);
      lineData.m_stopIds.emplace_back(stopId);
      m_stopIdMapping.emplace(stopIdSubway, stopId);

      if (i == 0)
        continue;

      auto const stopIdSubwayPrev = rangeSubway[i - 1];
      auto const & edge = FindEdge(stopIdSubwayPrev, stopIdSubway, lineId);

      for (auto const & id : edge.GetShapeIds())
      {
        auto const & polyline = GetShapeByStops(shapesSubway, id.GetStop1Id(), id.GetStop2Id());
        CHECK(!polyline.empty(), ());

        // We remove duplicate point from the shape before appending polyline to it.
        if (!shapeData.m_points.empty() && shapeData.m_points.back() == polyline.front())
          shapeData.m_points.pop_back();

        shapeData.m_points.insert(shapeData.m_points.end(), polyline.begin(), polyline.end());
      }

      EdgeId const curEdge(m_stopIdMapping[stopIdSubwayPrev], stopId, lineId);
      EdgePoints const edgePoints(shapeData.m_points.front(), shapeData.m_points.back());

      if (!m_edgesOnShapes.emplace(curEdge, edgePoints).second)
      {
        LOG(LWARNING, ("Edge duplicate in subways. stop1_id", stopIdSubwayPrev, "stop2_id",
                       stopIdSubway, "line_id", lineId));
      }
    }

    m_feed.m_lines.m_data.emplace(lineId, lineData);
    m_feed.m_shapes.m_data.emplace(shapeId, shapeData);
  }

  LOG(LDEBUG, ("Converted", m_feed.m_routes.m_data.size(), "routes,", m_feed.m_lines.m_data.size(),
               "lines from subways to public transport."));

  return !m_feed.m_lines.m_data.empty();
}

bool SubwayConverter::ConvertStops()
{
  auto const & stopsSubway = m_graphData.GetStops();
  m_feed.m_stops.m_data.reserve(stopsSubway.size());

  for (auto const & stopSubway : stopsSubway)
    m_feed.m_stops.m_data.emplace(MakeStop(stopSubway));

  LOG(LINFO,
      ("Converted", m_feed.m_stops.m_data.size(), "stops from subways to public transport."));

  return !m_feed.m_stops.m_data.empty();
}

bool SubwayConverter::ConvertTransfers()
{
  auto const & transfersSubway = m_graphData.GetTransfers();
  m_feed.m_transfers.m_data.reserve(transfersSubway.size());

  for (auto const & transferSubway : transfersSubway)
  {
    auto const [transferId, transferData] = MakeTransfer(transferSubway);
    m_feed.m_transfers.m_data.emplace(transferId, transferData);

    // All stops are already present in |m_feed| before the |ConvertTransfers()| call.
    for (auto stopId : transferData.m_stopsIds)
      LinkTransferIdToStop(m_feed.m_stops.m_data.at(stopId), transferId);
  }

  LOG(LINFO, ("Converted", m_feed.m_transfers.m_data.size(),
              "transfers from subways to public transport."));

  return !m_feed.m_transfers.m_data.empty();
}

bool SubwayConverter::ConvertGates()
{
  auto const & gatesSubway = m_graphData.GetGates();
  m_feed.m_gates.m_data.reserve(gatesSubway.size());

  for (auto const & gateSubway : gatesSubway)
  {
    auto const [gateId, gateData] = MakeGate(gateSubway);
    m_feed.m_gates.m_data.emplace(gateId, gateData);
  }

  LOG(LINFO,
      ("Converted", m_feed.m_gates.m_data.size(), "gates from subways to public transport."));

  return !m_feed.m_gates.m_data.empty();
}

bool SubwayConverter::ConvertEdges()
{
  for (auto const & edgeSubway : m_edgesSubway)
    m_feed.m_edges.m_data.emplace(MakeEdge(edgeSubway));

  LOG(LINFO,
      ("Converted", m_feed.m_edges.m_data.size(), "edges from subways to public transport."));

  for (auto const & edgeTransferSubway : m_edgesTransferSubway)
    m_feed.m_edgesTransfers.m_data.emplace(MakeEdgeTransfer(edgeTransferSubway));

  LOG(LINFO, ("Converted", m_feed.m_edgesTransfers.m_data.size(),
              "edges from subways to transfer edges in public transport."));

  return !m_feed.m_edges.m_data.empty() && !m_feed.m_edgesTransfers.m_data.empty();
}

routing::transit::Edge SubwayConverter::FindEdge(routing::transit::StopId stop1Id,
                                                 routing::transit::StopId stop2Id,
                                                 routing::transit::LineId lineId) const
{
  auto const itEdge = std::find_if(m_edgesSubway.begin(), m_edgesSubway.end(),
                                   [stop1Id, stop2Id, lineId](routing::transit::Edge const & edge) {
                                     return edge.GetStop1Id() == stop1Id &&
                                            edge.GetStop2Id() == stop2Id &&
                                            edge.GetLineId() == lineId;
                                   });

  CHECK(itEdge != m_edgesSubway.end(), (stop1Id, stop2Id, lineId));

  return *itEdge;
}
}  // namespace transit
