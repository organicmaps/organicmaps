#include "transit/world_feed/feed_helpers.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <cmath>

namespace
{
// Epsilon for m2::PointD comparison.
double constexpr kEps = 1e-5;

struct ProjectionData
{
  // Projection to polyline.
  m2::PointD m_proj;
  // Index before which the projection will be inserted.
  size_t m_indexOnShape = 0;
  // Distance from point to its projection.
  double m_distFromPoint = 0.0;
  // Distance from start point on polyline to the projection.
  double m_distFromStart = 0.0;
  // Point on polyline almost equal to the projection can already exist, so we don't need to
  // insert projection. Or we insert it to the polyline.
  bool m_needsInsertion = false;
};

// Returns true if |p1| is much closer to start then |p2| (parameter |distDeltaStart|) and its
// distance to projections to polyline |m_distFromPoint| is comparable.
bool CloserToStartAndOnSimilarDistToLine(ProjectionData const & p1, ProjectionData const & p2)
{
  // Delta between two points distances from start point on polyline.
  double constexpr distDeltaStart = 100.0;
  // Delta between two points distances from their corresponding projections to polyline.
  double constexpr distDeltaProj = 90.0;

  return (p1.m_distFromStart + distDeltaStart < p2.m_distFromStart &&
          std::abs(p2.m_distFromPoint - p1.m_distFromPoint) <= distDeltaProj);
}
}  // namespace

namespace transit
{
ProjectionToShape ProjectStopOnTrack(m2::PointD const & stopPoint, m2::PointD const & point1,
                                     m2::PointD const & point2)
{
  m2::PointD const stopProjection =
      m2::ParametrizedSegment<m2::PointD>(point1, point2).ClosestPointTo(stopPoint);
  double const distM = mercator::DistanceOnEarth(stopProjection, stopPoint);
  return {stopProjection, distM};
}

ProjectionData GetProjection(std::vector<m2::PointD> const & polyline, size_t index,
                             ProjectionToShape const & proj)
{
  ProjectionData projData;
  projData.m_distFromPoint = proj.m_dist;
  projData.m_proj = proj.m_point;

  if (base::AlmostEqualAbs(proj.m_point, polyline[index], kEps))
  {
    projData.m_indexOnShape = index;
    projData.m_needsInsertion = false;
  }
  else if (base::AlmostEqualAbs(proj.m_point, polyline[index + 1], kEps))
  {
    projData.m_indexOnShape = index + 1;
    projData.m_needsInsertion = false;
  }
  else
  {
    projData.m_indexOnShape = index + 1;
    projData.m_needsInsertion = true;
  }

  return projData;
}

void FillProjections(std::vector<m2::PointD> & polyline, size_t startIndex, size_t endIndex,
                     m2::PointD const & point, std::vector<ProjectionData> & projections)
{
  double distTravelledM = 0.0;
  // Stop can't be further from its projection to line then |maxDistFromStopM|.
  double constexpr maxDistFromStopM = 1000;

  for (size_t i = startIndex; i < endIndex; ++i)
  {
    if (i > startIndex)
      distTravelledM += mercator::DistanceOnEarth(polyline[i - 1], polyline[i]);

    auto proj = GetProjection(polyline, i, ProjectStopOnTrack(point, polyline[i], polyline[i + 1]));
    proj.m_distFromStart = distTravelledM + mercator::DistanceOnEarth(polyline[i], proj.m_proj);

    if (proj.m_distFromPoint < maxDistFromStopM)
      projections.emplace_back(proj);
  }
}

std::pair<size_t, bool> PrepareNearestPointOnTrack(m2::PointD const & point, size_t startIndex,
                                                   std::vector<m2::PointD> & polyline)
{
  std::vector<ProjectionData> projections;
  // Reserve space for points on polyline which are relatively close to the polyline.
  // Approximately 1/4 of all points on shape.
  projections.reserve(polyline.size() / 4);

  FillProjections(polyline, startIndex, polyline.size() - 1, point, projections);

  if (projections.empty())
    return {polyline.size() + 1, false};

  // We find the most fitting projection of the stop to the polyline. For two different projections
  // with approximately equal distances to the stop the most preferable is the one that is closer
  // to the beginning of the polyline segment.
  auto const proj =
      std::min_element(projections.begin(), projections.end(),
                       [](ProjectionData const & p1, ProjectionData const & p2) {
                         if (CloserToStartAndOnSimilarDistToLine(p1, p2))
                           return true;

                         if (CloserToStartAndOnSimilarDistToLine(p2, p1))
                           return false;

                         if (base::AlmostEqualAbs(p1.m_distFromPoint, p2.m_distFromPoint, kEps))
                           return p1.m_distFromStart < p2.m_distFromStart;

                         return p1.m_distFromPoint < p2.m_distFromPoint;
                       });

  if (proj->m_needsInsertion)
    polyline.insert(polyline.begin() + proj->m_indexOnShape, proj->m_proj);

  return {proj->m_indexOnShape, proj->m_needsInsertion};
}

bool IsRelevantType(const gtfs::RouteType & routeType)
{
  // All types and constants are described in GTFS:
  // https://developers.google.com/transit/gtfs/reference

  // We skip all subways because we extract subway data from OSM, not from GTFS.
  if (routeType == gtfs::RouteType::Subway)
    return false;

  auto const val = static_cast<size_t>(routeType);
  // "Classic" GTFS route types.
  if (val < 8 || (val > 10 && val < 13))
    return true;

  // Extended GTFS route types.
  // We do not handle taxi services.
  if (val >= 1500)
    return false;

  // Other not relevant types - school buses, lorry services etc.
  static std::vector<gtfs::RouteType> const kNotRelevantTypes{
      gtfs::RouteType::CarTransportRailService,
      gtfs::RouteType::LorryTransportRailService,
      gtfs::RouteType::VehicleTransportRailService,
      gtfs::RouteType::MetroService,
      gtfs::RouteType::UndergroundService,
      gtfs::RouteType::PostBusService,
      gtfs::RouteType::SpecialNeedsBus,
      gtfs::RouteType::MobilityBusService,
      gtfs::RouteType::MobilityBusForRegisteredDisabled,
      gtfs::RouteType::SchoolBus,
      gtfs::RouteType::SchoolAndPublicServiceBus};

  return std::find(kNotRelevantTypes.begin(), kNotRelevantTypes.end(), routeType) ==
         kNotRelevantTypes.end();
}

std::string ToString(gtfs::RouteType const & routeType)
{
  // GTFS route types.
  switch (routeType)
  {
  case gtfs::RouteType::Tram: return "tram";
  case gtfs::RouteType::Subway: return "subway";
  case gtfs::RouteType::Rail: return "rail";
  case gtfs::RouteType::Bus: return "bus";
  case gtfs::RouteType::Ferry: return "ferry";
  case gtfs::RouteType::CableTram: return "cable_tram";
  case gtfs::RouteType::AerialLift: return "aerial_lift";
  case gtfs::RouteType::Funicular: return "funicular";
  case gtfs::RouteType::Trolleybus: return "trolleybus";
  case gtfs::RouteType::Monorail: return "monorail";
  default:
    // Extended GTFS route types.
    return ToStringExtendedType(routeType);
  }
}

std::string ToStringExtendedType(gtfs::RouteType const & routeType)
{
  // These constants refer to extended GTFS routes types.
  auto const val = static_cast<size_t>(routeType);
  if (val >= 100 && val < 200)
    return "rail";

  if (val >= 200 && val < 300)
    return "bus";

  if (val == 405)
    return "monorail";

  if (val >= 400 && val < 500)
    return "rail";

  if (val >= 700 && val < 800)
    return "bus";

  if (val == 800)
    return "trolleybus";

  if (val >= 900 && val < 1000)
    return "tram";

  if (val == 1000)
    return "water_service";

  if (val == 1100)
    return "air_service";

  if (val == 1200)
    return "ferry";

  if (val == 1300)
    return "aerial_lift";

  if (val == 1400)
    return "funicular";

  LOG(LINFO, ("Unrecognized route type", val));
  return {};
}

gtfs::StopTimes GetStopTimesForTrip(gtfs::StopTimes const & allStopTimes,
                                    std::string const & tripId)
{
  gtfs::StopTime reference;
  reference.trip_id = tripId;

  auto itStart = std::lower_bound(
      allStopTimes.begin(), allStopTimes.end(), reference,
      [](const gtfs::StopTime & t1, const gtfs::StopTime & t2) { return t1.trip_id < t2.trip_id; });

  if (itStart == allStopTimes.end())
    return {};
  auto itEnd = itStart;
  while (itEnd != allStopTimes.end() && itEnd->trip_id == tripId)
    ++itEnd;

  gtfs::StopTimes res(itStart, itEnd);
  std::sort(res.begin(), res.end(), [](gtfs::StopTime const & t1, gtfs::StopTime const & t2) {
    return t1.stop_sequence < t2.stop_sequence;
  });
  return res;
}
}  // namespace transit
