#include "routing/road_info.hpp"

#include "routing/index_graph_loader.hpp"
#include "routing/speed_camera_prohibition.hpp"
#include "routing_common/car_model.hpp"

#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "geometry/mercator.hpp"
#include "indexer/feature.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace routing
{
std::optional<IRoadGraph::EdgeProjectionT> MatchRoad(m2::PointD const & position, m2::PointD const & direction,
                                                     double accuracy,
                                                     std::vector<IRoadGraph::EdgeProjectionT> const & candidates)
{
  if (direction.IsAlmostZero() || !std::isfinite(accuracy) || accuracy <= 0 || accuracy > 30)
    return {};
  struct Candidate
  {
    double m_score;
    IRoadGraph::EdgeProjectionT const * m_projection;
  };
  std::vector<Candidate> ranked;
  for (auto const & candidate : candidates)
  {
    auto const & edge = candidate.first;
    auto const d = edge.GetDirection();
    if (edge.IsFake() || d.IsAlmostZero())
      continue;
    double const cosine = DotProduct(d, direction) / (d.Length() * direction.Length());
    double const distance = mercator::DistanceOnEarth(position, candidate.second.GetPoint());
    if (cosine < 0.7 || distance > std::clamp(accuracy * 2.0, 10.0, 40.0))
      continue;
    ranked.push_back({distance + 5.0 * (1.0 - cosine), &candidate});
  }
  if (ranked.empty())
    return {};
  std::sort(ranked.begin(), ranked.end(), [](auto const & a, auto const & b) { return a.m_score < b.m_score; });
  auto const & best = *ranked.front().m_projection;
  for (size_t i = 1; i < ranked.size(); ++i)
  {
    auto const & edge = ranked[i].m_projection->first;
    if (edge.GetFeatureId() == best.first.GetFeatureId() && edge.IsForward() == best.first.IsForward() &&
        std::abs(static_cast<int64_t>(edge.GetSegId()) - best.first.GetSegId()) <= 1)
      continue;
    if (ranked[i].m_score - ranked.front().m_score < std::max(3.0, accuracy * 0.5))
      return {};
  }
  return best;
}

void FindRoadCamera(IRoadGraph const & graph, Edge edge, m2::PointD const & position, RoadCameraGetter const & cameras,
                    RoadInfoSnapshot & result)
{
  double const kHorizonMeters = 2000.0;
  double passed = -mercator::DistanceOnEarth(edge.GetStartPoint(), position);
  std::set<Edge> visited;
  for (size_t n = 0; n < 256 && passed < kHorizonMeters && visited.insert(edge).second; ++n)
  {
    double const length = mercator::DistanceOnEarth(edge.GetStartPoint(), edge.GetEndPoint());
    for (auto const & camera : cameras(edge))
    {
      double const coefficient = edge.IsForward() ? camera.m_coef : 1.0 - camera.m_coef;
      double const distance = passed + length * coefficient;
      if (distance < 0.0 || distance > kHorizonMeters ||
          (result.m_cameraDistance >= 0.0 && distance >= result.m_cameraDistance))
        continue;
      result.m_cameraDistance = distance;
      result.m_cameraLimitMps =
          camera.m_maxSpeedKmPH == SpeedCameraOnRoute::kNoSpeedInfo ? 0.0 : camera.m_maxSpeedKmPH / 3.6;
      result.m_cameraPosition = edge.GetStartPoint() + edge.GetDirection() * coefficient;
    }
    if (result.m_cameraDistance >= 0.0)
      return;
    passed += length;
    IRoadGraph::EdgeListT outgoing;
    graph.GetOutgoingEdges(edge.GetEndJunction(), outgoing);
    std::set<Edge> choices;
    for (auto const & next : outgoing)
      if (!next.IsFake() && next != edge.GetReverseEdge())
        choices.insert(next);
    // No route: do not guess the driver's choice at a junction.
    if (choices.size() != 1)
      return;
    edge = *choices.begin();
  }
}

RoadInfoReader::RoadInfoReader(DataSource & source, VehicleModelFactory::CountryParentNameGetterFn const & parents)
  : m_source(source, nullptr)
  , m_graph(m_source, IRoadGraph::Mode::ObeyOnewayTag, std::make_shared<CarModelFactory>(parents))
{}

RoadInfoReader::Attributes & RoadInfoReader::GetAttributes(MwmSet::MwmId const & id)
{
  auto [it, inserted] = m_attributes.try_emplace(id);
  if (inserted)
  {
    auto const & handle = m_source.GetHandle(id);
    it->second.m_speeds = LoadMaxspeeds(handle);
    if (!AreSpeedCamerasProhibited(handle.GetInfo()->GetLocalFile().GetCountryFile()))
      ReadSpeedCamsFromMwm(*handle.GetValue(), it->second.m_cameras);
  }
  return it->second;
}

RoadInfoSnapshot RoadInfoReader::Read(location::GpsInfo const & location)
{
  // Do not pin map files across updates/removals. Attribute caches are keyed by MwmId.
  SCOPE_GUARD(release, [this] { m_source.FreeHandles(); });
  RoadInfoSnapshot result;
  double const time = location.m_timestamp;
  if (!std::isfinite(time) || !std::isfinite(location.m_latitude) || !std::isfinite(location.m_longitude) ||
      std::abs(location.m_latitude) > 85.0 || std::abs(location.m_longitude) > 180.0)
    return result;
  auto const position = mercator::FromLatLon(location.m_latitude, location.m_longitude);
  bool const continuous =
      m_previousLocation && time > m_previousLocation->m_timestamp && time - m_previousLocation->m_timestamp <= 5.0 &&
      mercator::DistanceOnEarth(
          position, mercator::FromLatLon(m_previousLocation->m_latitude, m_previousLocation->m_longitude)) < 200.0;
  if (!continuous)
  {
    m_previousMatch.reset();
    m_direction = {};
  }
  if (location.HasBearing() && std::isfinite(location.m_bearing) && location.m_speed >= 1.0)
  {
    double const angle = math::DegToRad(location.m_bearing);
    m_direction = {std::sin(angle), std::cos(angle)};
    m_directionTime = time;
  }
  else if (continuous)
  {
    auto const previous = mercator::FromLatLon(m_previousLocation->m_latitude, m_previousLocation->m_longitude);
    if (mercator::DistanceOnEarth(position, previous) >= std::max(5.0, location.m_horizontalAccuracy))
    {
      m_direction = position - previous;
      m_directionTime = time;
    }
    else if (m_previousMatch && location.m_speed >= 0.0 && location.m_speed < 1.0)
    {
      // A fresh stationary fix does not invalidate the approach direction at a traffic light.
      m_directionTime = time;
    }
  }
  m_previousLocation = location;
  if (time - m_directionTime > 10.0)
    m_direction = {};
  if (m_attributes.size() > 4)
  {
    m_attributes.clear();
    m_graph.ClearState();
  }
  std::erase_if(m_attributes, [](auto const & entry) { return !entry.first.IsAlive(); });
  std::vector<IRoadGraph::EdgeProjectionT> candidates;
  m_graph.FindClosestEdges(mercator::RectByCenterXYAndSizeInMeters(position, 40.0), 16, candidates);
  auto match = MatchRoad(position, m_direction, location.m_horizontalAccuracy, candidates);
  bool const stable = match && m_previousMatch &&
                      match->first.GetFeatureId() == m_previousMatch->first.GetFeatureId() &&
                      match->first.IsForward() == m_previousMatch->first.IsForward();
  m_previousMatch = match;
  if (!stable)
    return result;
  auto const & edge = match->first;
  auto & attrs = GetAttributes(edge.GetFeatureId().m_mwmId);
  result.m_matched = true;
  if (attrs.m_speeds)
  {
    auto const speed = attrs.m_speeds->GetMaxspeed(edge.GetFeatureId().m_index)
                           .GetCurrentSpeed(static_cast<time_t>(time), edge.IsForward());
    if (speed.IsNumeric())
      result.m_speedLimitMps = speed.GetSpeedKmPH() / 3.6;
  }
  if (auto feature = m_source.GetFeature(edge.GetFeatureId()))
    result.m_road = feature->GetReadableName();
  FindRoadCamera(m_graph, edge, match->second.GetPoint(), [this](Edge const & e)
  {
    auto const & cameras = GetAttributes(e.GetFeatureId().m_mwmId).m_cameras;
    auto const it = cameras.find({e.GetFeatureId().m_index, e.GetSegId()});
    return it == cameras.end() ? std::vector<RouteSegment::SpeedCamera>{} : it->second;
  }, result);
  return result;
}
}  // namespace routing
