#pragma once

#include "generator/affiliation.hpp"

#include "transit/transit_entities.hpp"
#include "transit/transit_graph_data.hpp"
#include "transit/world_feed/world_feed.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "defines.hpp"

#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "3party/opening_hours/opening_hours.hpp"

namespace transit
{
// Pair of points representing corresponding edge endings.
using EdgePoints = std::pair<m2::PointD, m2::PointD>;

// Converts public transport data from the MAPS.ME old transit.json format (which contains only
// subway data) to the new line-by-line jsons used for handling data extracted from GTFS.
class SubwayConverter
{
public:
  SubwayConverter(std::string const & subwayJson, WorldFeed & feed);
  // Parses subway json and converts it to entities in WorldFeed |m_feed|.
  bool Convert();

private:
  bool ConvertNetworks();
  // Splits subway edges in two containers: |m_edgesSubway| for edges between two stops on the line
  // and |m_edgesTransferSubway| for transfer edges.
  bool SplitEdges();
  // Converts lines, creates routes based on the lines data and constructs shapes.
  bool ConvertLinesBasedData();
  bool ConvertStops();
  bool ConvertTransfers();
  bool ConvertGates();
  bool ConvertEdges();

  // Methods for creating id & data pairs for |m_feed| based on the subway items.
  std::pair<TransitId, RouteData> MakeRoute(routing::transit::Line const & lineSubway);
  std::pair<TransitId, GateData> MakeGate(routing::transit::Gate const & gateSubway);
  std::pair<TransitId, TransferData> MakeTransfer(
      routing::transit::Transfer const & transferSubway);
  std::pair<TransitId, LineData> MakeLine(routing::transit::Line const & lineSubway,
                                          TransitId routeId);
  std::pair<EdgeId, EdgeData> MakeEdge(routing::transit::Edge const & edgeSubway);
  std::pair<EdgeTransferId, size_t> MakeEdgeTransfer(routing::transit::Edge const & edgeSubway);
  std::pair<TransitId, StopData> MakeStop(routing::transit::Stop const & stopSubway);

  routing::transit::Edge FindEdge(routing::transit::StopId stop1Id,
                                  routing::transit::StopId stop2Id,
                                  routing::transit::LineId lineId) const;

  // Path to the file with subways json.
  std::string m_subwayJson;
  // Transit graph for deserializing json from |m_subwayJson|.
  routing::transit::GraphData m_graphData;
  // Destination feed for converted items from subway.
  WorldFeed & m_feed;
  // Mapping of subway stop id to transit stop id.
  std::unordered_map<routing::transit::StopId, TransitId> m_stopIdMapping;
  // Subset of the |m_graphData| edges with no transfers.
  std::vector<routing::transit::Edge> m_edgesSubway;
  // Subset of the |m_graphData| edges with transfers.
  std::vector<routing::transit::Edge> m_edgesTransferSubway;
  // Mapping of the edge to its ending points on the shape polyline.
  std::unordered_map<EdgeId, EdgePoints, EdgeIdHasher> m_edgesOnShapes;
};
}  // namespace transit
