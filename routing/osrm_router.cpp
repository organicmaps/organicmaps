#include "osrm_router.hpp"
#include "osrm_data_facade.hpp"
#include "route.hpp"

#include "../indexer/mercator.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/SearchEngine.h"
#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"
#include "../3party/osrm/osrm-backend/Descriptors/DescriptionFactory.h"

namespace routing
{

#define FACADE_READ_ZOOM_LEVEL  13

string OsrmRouter::GetName() const
{
  return "mapsme";
}

void OsrmRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  m_finalPt = finalPt;
}

void OsrmRouter::CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback)
{
  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT facade("/Users/deniskoronchik/Documents/develop/omim-maps/Belarus.osrm");
  SearchEngine<DataFacadeT> engine(&facade);

  RawRouteData rawRoute;
  PhantomNodes nodes;
  FixedPointCoordinate startPoint((int)(MercatorBounds::YToLat(startingPt.y) * COORDINATE_PRECISION),
                                  (int)(MercatorBounds::XToLon(startingPt.x) * COORDINATE_PRECISION));
  facade.FindPhantomNodeForCoordinate(startPoint, nodes.source_phantom, 18);
                                      //FACADE_READ_ZOOM_LEVEL);
  FixedPointCoordinate endPoint((int)(MercatorBounds::YToLat(m_finalPt.y) * COORDINATE_PRECISION),
                                (int)(MercatorBounds::XToLon(m_finalPt.x) * COORDINATE_PRECISION));
  facade.FindPhantomNodeForCoordinate(endPoint, nodes.target_phantom, 18);
                                      //FACADE_READ_ZOOM_LEVEL);

  rawRoute.raw_via_node_coordinates = {startPoint, endPoint};
  rawRoute.segment_end_coordinates.push_back(nodes);

  engine.shortest_path({nodes}, {}, rawRoute);

  Route route(GetName());
  if (INVALID_EDGE_WEIGHT == rawRoute.shortest_path_length
      || rawRoute.segment_end_coordinates.empty()
      || rawRoute.source_traversed_in_reverse.empty())
  {
    callback(route);
    return;
  }

  // restore route
  DescriptionFactory factory;
  factory.SetStartSegment(rawRoute.segment_end_coordinates.front().source_phantom,
                          rawRoute.source_traversed_in_reverse.front());

  // for each unpacked segment add the leg to the description
  for (auto const i : osrm::irange<std::size_t>(0, rawRoute.unpacked_path_segments.size()))
  {
      unsigned added_element_count = 0;
      // Get all the coordinates for the computed route
      FixedPointCoordinate current_coordinate;
      for (PathData const & path_data : rawRoute.unpacked_path_segments[i])
      {
          current_coordinate = facade.GetCoordinateOfNode(path_data.node);
          factory.AppendSegment(current_coordinate, path_data);
          ++added_element_count;
      }
      factory.SetEndSegment(rawRoute.segment_end_coordinates[i].target_phantom, rawRoute.target_traversed_in_reverse[i]);
      ++added_element_count;
      ASSERT_EQUAL((rawRoute.unpacked_path_segments[i].size() + 1), added_element_count, ());
  }
  factory.Run(&facade, FACADE_READ_ZOOM_LEVEL);
  factory.BuildRouteSummary(factory.entireLength, rawRoute.shortest_path_length);

  vector<m2::PointD> points;
  for (auto const si : factory.path_description)
    points.push_back(m2::PointD(MercatorBounds::LonToX(si.location.lon / COORDINATE_PRECISION), MercatorBounds::LatToY(si.location.lat / COORDINATE_PRECISION))); // @todo: possible need convert to mercator
  route.SetGeometry(points.begin(), points.end());

  callback(route);
}

}
