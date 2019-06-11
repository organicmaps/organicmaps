#include "generator/streets/street_regions_tracing.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <iterator>

#include <boost/optional.hpp>

namespace generator
{
namespace streets
{
constexpr m2::Meter const StreetRegionsTracing::kRegionCheckStepDistance;
constexpr m2::Meter const StreetRegionsTracing::kRegionBoundarySearchStepDistance;

StreetRegionsTracing::StreetRegionsTracing(Path const & path,
                                           StreetRegionInfoGetter const & streetRegionInfoGetter)
  : m_path{path}, m_streetRegionInfoGetter{streetRegionInfoGetter}
{
  CHECK_GREATER_OR_EQUAL(m_path.size(), 2, ());

  m_currentPoint = m_path.front();
  m_nextPathPoint = std::next(m_path.begin());

  m_currentRegion = m_streetRegionInfoGetter(m_currentPoint);
  if (m_currentRegion)
    StartNewSegment();

  Trace();
}

StreetRegionsTracing::PathSegments && StreetRegionsTracing::StealPathSegments()
{
  return std::move(m_pathSegments);
}

void StreetRegionsTracing::Trace()
{
  while (m_nextPathPoint != m_path.end())
  {
    if (!TraceToNextCheckPointInCurrentRegion())
      TraceUpToNextRegion();
  }

  if (m_currentRegion)
    ReleaseCurrentSegment();
}

bool StreetRegionsTracing::TraceToNextCheckPointInCurrentRegion()
{
  Meter distanceToCheckPoint{};
  auto newNextPathPoint = m_nextPathPoint;
  auto checkPoint = FollowToNextPoint(m_currentPoint, kRegionCheckStepDistance, m_nextPathPoint,
                                      distanceToCheckPoint, newNextPathPoint);

  auto checkPointRegion = m_streetRegionInfoGetter(checkPoint);
  if (!IsSameRegion(checkPointRegion))
    return false;

  AdvanceTo(checkPoint, distanceToCheckPoint, newNextPathPoint);
  return true;
}

void StreetRegionsTracing::TraceUpToNextRegion()
{
  while (m_nextPathPoint != m_path.end())
  {
    Meter distanceToNextPoint{};
    auto newNextPathPoint = m_nextPathPoint;
    auto nextPoint = FollowToNextPoint(m_currentPoint, kRegionBoundarySearchStepDistance, m_nextPathPoint,
                                       distanceToNextPoint, newNextPathPoint);

    auto nextPointRegion = m_streetRegionInfoGetter(nextPoint);
    if (!IsSameRegion(nextPointRegion))
    {
      AdvanceToBoundary(nextPointRegion, nextPoint, distanceToNextPoint, newNextPathPoint);
      return;
    }

    AdvanceTo(nextPoint, distanceToNextPoint, newNextPathPoint);
  }
}

void StreetRegionsTracing::AdvanceToBoundary(boost::optional<KeyValue> const & newRegion,
    m2::PointD const newRegionPoint, Meter distance, Path::const_iterator nextPathPoint)
{
  if (m_currentRegion)
    CloseCurrentSegment(newRegionPoint, distance, nextPathPoint);

  m_currentRegion = newRegion;
  if (m_currentRegion)
    StartNewSegment();

  AdvanceTo(newRegionPoint, distance, nextPathPoint);
}

void StreetRegionsTracing::AdvanceTo(m2::PointD const toPoint, Meter distance,
                                     Path::const_iterator nextPathPoint)
{
  CHECK(0.0 <= distance || m_nextPathPoint != nextPathPoint, ());

  if (m_currentRegion)
  {
    CHECK(!m_pathSegments.empty(), ());
    auto & currentSegment = m_pathSegments.back();
    std::copy(m_nextPathPoint, nextPathPoint, std::back_inserter(currentSegment.m_path));
    currentSegment.m_pathLengthMeters += distance;
  }

  m_currentPoint = toPoint;
  m_nextPathPoint = nextPathPoint;
}

void StreetRegionsTracing::StartNewSegment()
{
  CHECK(m_currentRegion, ());
  m_pathSegments.push_back({*m_currentRegion, {m_currentPoint}, 0.0 /* m_pathLengthMeters */});
}

void StreetRegionsTracing::CloseCurrentSegment(m2::PointD const endPoint, Meter distance,
                                               Path::const_iterator pathEndPoint)
{
  CHECK(0.0 <= distance || m_nextPathPoint != pathEndPoint, ());

  CHECK(!m_pathSegments.empty(), ());
  auto & currentSegment = m_pathSegments.back();

  std::copy(m_nextPathPoint, pathEndPoint, std::back_inserter(currentSegment.m_path));
  if (currentSegment.m_path.back() != endPoint)
    currentSegment.m_path.push_back(endPoint);

  currentSegment.m_pathLengthMeters += distance;

  ReleaseCurrentSegment();
}

void StreetRegionsTracing::ReleaseCurrentSegment()
{
  CHECK(!m_pathSegments.empty(), ());
  auto & currentSegment = m_pathSegments.back();

  CHECK_GREATER_OR_EQUAL(currentSegment.m_path.size(), 2, ());

  constexpr auto kSegmentLengthMin = 1.0_m;
  if (m_pathSegments.back().m_pathLengthMeters < kSegmentLengthMin)
    m_pathSegments.pop_back();
}

m2::PointD StreetRegionsTracing::FollowToNextPoint(m2::PointD const & startPoint, Meter stepDistance,
    Path::const_iterator nextPathPoint, Meter & distance, Path::const_iterator & newNextPathPoint) const
{
  auto point = startPoint;
  distance = 0.0_m;
  newNextPathPoint = nextPathPoint;
  for (; newNextPathPoint != m_path.end(); ++newNextPathPoint)
  {
    auto nextPoint = *newNextPathPoint;
    auto distanceToNextPoint = Meter{MercatorBounds::DistanceOnEarth(point, nextPoint)};
    CHECK_LESS(distance, stepDistance, ());
    auto restDistance = stepDistance - distance;

    constexpr auto kDistancePrecision = 2.0_m;
    if (std::fabs(restDistance - distanceToNextPoint) <= kDistancePrecision)
    {
      ++newNextPathPoint;
      distance += distanceToNextPoint;
      return nextPoint;
    }

    if (restDistance < distanceToNextPoint)
    {
      point += (nextPoint - point) * restDistance / distanceToNextPoint;
      distance = stepDistance;
      return point;
    }

    point = nextPoint;
    distance += distanceToNextPoint;
  }

  return point;
}

bool StreetRegionsTracing::IsSameRegion(boost::optional<KeyValue> const & region) const
{
  if (m_currentRegion && region)
    return m_currentRegion->first == region->first;

  return !m_currentRegion && !region;
}
}  // namespace streets
}  // namespace generator
