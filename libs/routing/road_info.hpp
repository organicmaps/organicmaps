#pragma once

#include "routing/data_source.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/speed_camera_ser_des.hpp"

#include "platform/location.hpp"

#include <optional>

namespace routing
{
struct RoadInfoSnapshot
{
  bool m_matched = false;
  double m_speedLimitMps = 0.0;
  std::string m_road;
  double m_cameraDistance = -1.0;
  double m_cameraLimitMps = 0.0;
  m2::PointD m_cameraPosition;
};

// Pure selection policy, shared with tests. Ambiguous parallel roads are not a match.
std::optional<IRoadGraph::EdgeProjectionT> MatchRoad(m2::PointD const & position, m2::PointD const & direction,
                                                     double accuracy,
                                                     std::vector<IRoadGraph::EdgeProjectionT> const & candidates);

using RoadCameraGetter = std::function<std::vector<RouteSegment::SpeedCamera>(Edge const &)>;
void FindRoadCamera(IRoadGraph const & graph, Edge edge, m2::PointD const & position, RoadCameraGetter const & cameras,
                    RoadInfoSnapshot & result);

// Owned and called by a single location worker, independently of the route and renderers.
class RoadInfoReader
{
public:
  RoadInfoReader(DataSource & source, VehicleModelFactory::CountryParentNameGetterFn const & parents);
  RoadInfoSnapshot Read(location::GpsInfo const & location);

private:
  struct Attributes
  {
    std::unique_ptr<Maxspeeds> m_speeds;
    SpeedCamerasMapT m_cameras;
  };
  Attributes & GetAttributes(MwmSet::MwmId const & id);
  MwmDataSource m_source;
  FeaturesRoadGraphBase m_graph;
  std::map<MwmSet::MwmId, Attributes> m_attributes;
  std::optional<IRoadGraph::EdgeProjectionT> m_previousMatch;
  std::optional<location::GpsInfo> m_previousLocation;
  m2::PointD m_direction;
  double m_directionTime = 0.0;
};
}  // namespace routing
