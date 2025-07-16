#pragma once

#include "generator/affiliation.hpp"

#include "transit/transit_entities.hpp"
#include "transit/transit_graph_data.hpp"
#include "transit/world_feed/feed_helpers.hpp"
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
using LineIdToStops = std::unordered_map<TransitId, IdList>;
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
  // Tries to minimize the reversed lines count where it is possible by reversing line geometry.
  void MinimizeReversedLinesCount();
  // Returns line ids with corresponding shape links and route ids. There can be may lines inside
  // the route with same shapeLink. We keep only one of them. These line ids are used in
  // |PrepareLinesMetadata()|.
  std::vector<LineSchemeData> GetLinesOnScheme(
      std::unordered_map<TransitId, LineSegmentInRegion> const & linesInRegion) const;
  // Finds common overlapping (parallel on the subway layer) segments on polylines. Motivation:
  // we shouldn't draw parallel lines of different routes on top of each other so the user can’t
  // tell which lines go where (the only visible line is the one that is drawn last). We need these
  // lines to be drawn in parallel in corresponding routes colours.
  void PrepareLinesMetadata();
  // Calculates order for each of the parallel lines in the overlapping segment. In drape frontend
  // we use this order as an offset for drawing line.
  void CalculateLinePriorities(std::vector<LineSchemeData> const & linesOnScheme);

  // Methods for creating id & data pairs for |m_feed| based on the subway items.
  std::pair<TransitId, RouteData> MakeRoute(routing::transit::Line const & lineSubway);
  std::pair<TransitId, GateData> MakeGate(routing::transit::Gate const & gateSubway);
  std::pair<TransitId, TransferData> MakeTransfer(routing::transit::Transfer const & transferSubway);
  static std::pair<TransitId, LineData> MakeLine(routing::transit::Line const & lineSubway, TransitId routeId);
  std::pair<EdgeId, EdgeData> MakeEdge(routing::transit::Edge const & edgeSubway, uint32_t index);
  std::pair<EdgeTransferId, EdgeData> MakeEdgeTransfer(routing::transit::Edge const & edgeSubway, uint32_t index);
  std::pair<TransitId, StopData> MakeStop(routing::transit::Stop const & stopSubway);

  routing::transit::Edge FindEdge(routing::transit::StopId stop1Id, routing::transit::StopId stop2Id,
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
  std::map<routing::transit::Edge, uint32_t> m_edgesSubway;
  // Subset of the |m_graphData| edges with transfers.
  std::map<routing::transit::Edge, uint32_t> m_edgesTransferSubway;
};
}  // namespace transit
