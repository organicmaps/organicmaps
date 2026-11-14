#include "map/track_follow_data.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>
#include <utility>

namespace
{
using SourcePosition = TrackSourcePosition;
using PWA = geometry::PointWithAltitude;

// Fractions this close to the endpoint identify the next source segment.
double constexpr kEndpointFractionEps = 1e-12;
// Map/graph distance rounding must still identify the same source occurrence.
double constexpr kSelectionDistanceToleranceM = 0.01;
// A repeated legacy stop needs real movement away from its previous visit.
double constexpr kDistinctStopVisitM = 1.0;
// Treat projections within this distance as equally close to a repeated stop.
double constexpr kStopProjectionToleranceM = 0.5;
// A geometric follow cannot visit a stop farther from its source geometry.
double constexpr kMaxStopDistanceM = 5.0;

bool IsBefore(SourcePosition const & lhs, SourcePosition const & rhs)
{
  return std::tie(lhs.m_line, lhs.m_segment, lhs.m_fraction) < std::tie(rhs.m_line, rhs.m_segment, rhs.m_fraction);
}

SourcePosition NormalizePosition(kml::MultiGeometry const & geometry, SourcePosition position, bool joinLines = true)
{
  if (position.m_fraction < 1.0 - kEndpointFractionEps)
    return position;
  position.m_fraction = 0.0;
  ++position.m_segment;
  auto const & line = geometry.m_lines[position.m_line];
  if (position.m_segment + 1 < line.size())
    return position;
  if (joinLines && position.m_line + 1 < geometry.m_lines.size())
  {
    auto const & next = geometry.m_lines[position.m_line + 1];
    if (next.size() > 1 && line.back().GetPoint().EqualDxDy(next.front().GetPoint(), mercator::kPointEqualityEps))
      return {position.m_line + 1, 0, 0.0};
  }
  --position.m_segment;
  position.m_fraction = 1.0;
  return position;
}

PWA PointAtPosition(kml::MultiGeometry const & geometry, SourcePosition const & position)
{
  auto const & line = geometry.m_lines[position.m_line];
  auto const & from = line[position.m_segment];
  auto const & to = line[position.m_segment + 1];
  if (position.m_fraction <= 0.0)
    return from;
  if (position.m_fraction >= 1.0)
    return to;
  auto result = from.Interpolate(to, position.m_fraction);
  if (from.GetAltitude() == geometry::kInvalidAltitude || to.GetAltitude() == geometry::kInvalidAltitude)
    result.SetAltitude(geometry::kInvalidAltitude);
  return result;
}

struct StopProjection
{
  SourcePosition m_position;
  double m_distanceSq = std::numeric_limits<double>::max();
};

std::optional<StopProjection> ProjectStop(kml::MultiGeometry const & geometry, m2::PointD const & point,
                                          SourcePosition const & lowerBound, bool requireReturn = false)
{
  std::optional<StopProjection> best;
  auto bound = lowerBound;
  while (bound.m_line < geometry.m_lines.size() && bound.m_segment + 1 >= geometry.m_lines[bound.m_line].size())
    bound = {bound.m_line + 1, 0, 0.0};
  if (bound.m_line >= geometry.m_lines.size())
    return std::nullopt;
  auto const previousAnchor = PointAtPosition(geometry, bound).GetPoint();
  bool leftPrevious = false;
  for (size_t lineIndex = bound.m_line; lineIndex < geometry.m_lines.size(); ++lineIndex)
  {
    auto const & line = geometry.m_lines[lineIndex];
    size_t const firstSegment = lineIndex == bound.m_line ? bound.m_segment : 0;
    for (size_t segment = firstSegment; segment + 1 < line.size(); ++segment)
    {
      auto const & from = line[segment].GetPoint();
      auto const & to = line[segment + 1].GetPoint();
      auto const direction = to - from;
      double const minFraction = lineIndex == bound.m_line && segment == bound.m_segment ? bound.m_fraction : 0.0;
      double const fraction =
          direction.SquaredLength() > 0.0
              ? math::Clamp(m2::DotProduct(point - from, direction) / direction.SquaredLength(), minFraction, 1.0)
              : minFraction;
      auto const projected = from + direction * fraction;
      auto const fromBound = from + direction * minFraction;
      leftPrevious = leftPrevious || mercator::DistanceOnEarth(previousAnchor, fromBound) > kDistinctStopVisitM;
      double const distanceSq = projected.SquaredLength(point);
      if ((!requireReturn || leftPrevious) && (!best || distanceSq < best->m_distanceSq))
      {
        best = StopProjection{NormalizePosition(geometry, {lineIndex, segment, fraction}), distanceSq};
        if (distanceSq == 0.0)
          return best;
      }
      leftPrevious = leftPrevious || mercator::DistanceOnEarth(previousAnchor, to) > kDistinctStopVisitM;
    }
  }
  return best;
}
}  // namespace

std::optional<TrackSourcePosition> ResolveTrackEntry(kml::MultiGeometry const & geometry,
                                                     m2::PointD const & selectedPoint, double distanceFromBeginningM)
{
  if (!std::isfinite(distanceFromBeginningM) || distanceFromBeginningM < 0.0)
    return std::nullopt;

  double distance = 0.0;
  for (size_t lineIndex = 0; lineIndex < geometry.m_lines.size(); ++lineIndex)
  {
    auto const & line = geometry.m_lines[lineIndex];
    for (size_t segment = 0; segment + 1 < line.size(); ++segment)
    {
      auto const & from = line[segment].GetPoint();
      auto const & to = line[segment + 1].GetPoint();
      double const length = mercator::DistanceOnEarth(from, to);
      m2::ParametrizedSegment<m2::PointD> const edge(from, to);
      auto const projected = edge.ClosestPointTo(selectedPoint);
      auto const delta = to - from;
      double const fraction = delta.SquaredLength() > 0.0
                                ? math::Clamp(m2::DotProduct(projected - from, delta) / delta.SquaredLength(), 0.0, 1.0)
                                : 0.0;
      // Map taps accumulate geodesic distance to the selected coordinate; graph scrubs use
      // a fraction of the complete segment's length. Both identify the same source occurrence.
      double const selectedDistance = distance + mercator::DistanceOnEarth(from, projected);
      if (projected.EqualDxDy(selectedPoint, mercator::kPointEqualityEps) &&
          (AlmostEqualAbs(distanceFromBeginningM, selectedDistance, kSelectionDistanceToleranceM) ||
           AlmostEqualAbs(distanceFromBeginningM, distance + length * fraction, kSelectionDistanceToleranceM)))
      {
        return NormalizePosition(geometry, {lineIndex, segment, fraction});
      }
      distance += length;
    }
  }
  return std::nullopt;
}

std::expected<routing::TrackFollowingData, FollowTrackResult> BuildTrackFollowingData(kml::TrackData const & source,
                                                                                      TrackSourcePosition const & entry,
                                                                                      bool reverse)
{
  routing::TrackFollowingData result;
  auto const & geometry = source.m_geometry;
  if (entry.m_line >= geometry.m_lines.size() || entry.m_segment + 1 >= geometry.m_lines[entry.m_line].size())
    return std::unexpected(FollowTrackResult::InvalidSelection);

  size_t firstLine = entry.m_line;
  size_t lastLine = entry.m_line;
  while (reverse && firstLine > 0)
  {
    auto const & previous = geometry.m_lines[firstLine - 1];
    if (previous.size() < 2 || !previous.back().GetPoint().EqualDxDy(geometry.m_lines[firstLine].front().GetPoint(),
                                                                     mercator::kPointEqualityEps))
      break;
    --firstLine;
  }
  while (!reverse && lastLine + 1 < geometry.m_lines.size())
  {
    auto const & next = geometry.m_lines[lastLine + 1];
    if (next.size() < 2 ||
        !geometry.m_lines[lastLine].back().GetPoint().EqualDxDy(next.front().GetPoint(), mercator::kPointEqualityEps))
      break;
    ++lastLine;
  }

  struct Event
  {
    SourcePosition m_position;
    PWA m_point;
    bool m_stop = false;
  };
  std::vector<Event> events;
  events.push_back({entry, PointAtPosition(geometry, entry)});

  auto const inRemainder = [&](SourcePosition const & position)
  {
    return position.m_line >= firstLine && position.m_line <= lastLine &&
           (reverse ? !IsBefore(entry, position) : !IsBefore(position, entry));
  };
  for (size_t lineIndex = firstLine; lineIndex <= lastLine; ++lineIndex)
  {
    auto const & line = geometry.m_lines[lineIndex];
    for (size_t pointIndex = 0; pointIndex < line.size(); ++pointIndex)
    {
      auto const position =
          NormalizePosition(geometry,
                            pointIndex + 1 < line.size() ? SourcePosition{lineIndex, pointIndex, 0.0}
                                                         : SourcePosition{lineIndex, pointIndex - 1, 1.0},
                            false);
      if (inRemainder(position))
        events.push_back({position, line[pointIndex]});
    }
  }

  bool const hasGeometry = std::any_of(events.begin(), events.end(), [&](Event const & event)
  { return !event.m_point.GetPoint().EqualDxDy(events.front().m_point.GetPoint(), mercator::kPointEqualityEps); });
  if (!hasGeometry)
    return std::unexpected(FollowTrackResult::NoRemainingGeometry);

  auto const geometryEnd = events.size();
  SourcePosition previousStop;
  std::optional<m2::PointD> previousStopPoint;
  for (auto const & stop : source.m_stops)
  {
    auto projected = ProjectStop(geometry, stop.m_point, previousStop);
    if (!projected)
      return std::unexpected(FollowTrackResult::InvalidSelection);
    // Legacy stops have coordinates/order only. Prefer a later return after real intervening
    // geometry for repeated coordinates; retain the same occurrence for collocated duplicates
    // when no equally close later visit exists.
    if (previousStopPoint && previousStopPoint->EqualDxDy(stop.m_point, mercator::kPointEqualityEps))
    {
      if (auto const later = ProjectStop(geometry, stop.m_point, previousStop, true))
      {
        double const firstDistance =
            mercator::DistanceOnEarth(PointAtPosition(geometry, projected->m_position).GetPoint(), stop.m_point);
        double const laterDistance =
            mercator::DistanceOnEarth(PointAtPosition(geometry, later->m_position).GetPoint(), stop.m_point);
        if (laterDistance <= firstDistance + kStopProjectionToleranceM)
          projected = later;
      }
    }
    previousStop = projected->m_position;
    previousStopPoint = stop.m_point;
    if (!inRemainder(projected->m_position))
      continue;
    auto const anchor = PointAtPosition(geometry, projected->m_position);
    // A geometric follow cannot promise a visit to a stop far away from its source line.
    if (mercator::DistanceOnEarth(anchor.GetPoint(), stop.m_point) > kMaxStopDistanceM)
      return std::unexpected(FollowTrackResult::InvalidSelection);
    events.push_back({projected->m_position, anchor, true});
  }

  auto const stopsBegin = events.begin() + geometryEnd;
  if (reverse)
  {
    // Keep the selected entry first while reversing the remaining source samples.
    std::reverse(events.begin() + 1, stopsBegin);
    std::reverse(stopsBegin, events.end());
  }
  auto const bySourcePosition = [reverse](Event const & lhs, Event const & rhs)
  { return reverse ? IsBefore(rhs.m_position, lhs.m_position) : IsBefore(lhs.m_position, rhs.m_position); };
  ASSERT(std::is_sorted(events.begin(), stopsBegin, bySourcePosition), ());
  ASSERT(std::is_sorted(stopsBegin, events.end(), bySourcePosition), ());
  // Geometry and projected stops each preserve source order, including collocated events.
  std::inplace_merge(events.begin(), stopsBegin, events.end(), bySourcePosition);
  for (auto const & event : events)
  {
    if (result.m_points.empty() || result.m_points.back() != event.m_point)
      result.m_points.push_back(event.m_point);
    if (event.m_stop)
      result.m_stopPointIndexes.push_back(result.m_points.size() - 1);
  }
  return result;
}
