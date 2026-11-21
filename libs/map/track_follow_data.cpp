#include "map/track_follow_data.hpp"

#include "map/track.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace
{
using SourcePosition = TrackSourcePosition;
using PWA = geometry::PointWithAltitude;

// Fractions this close to the endpoint identify the next source segment.
double constexpr kEndpointFractionEps = 1e-12;
// Map/graph distance rounding must still identify the same source occurrence.
double constexpr kSelectionDistanceToleranceM = 0.01;
// Separate visits to the same source vertex must include movement away from it.
double constexpr kDistinctStopVisitM = 1.0;
// Repeated stops may differ slightly in their proximity to the source on separate visits.
double constexpr kStopProjectionToleranceM = 0.5;
// Stops farther from the source line cannot be visited by geometric navigation.
double constexpr kMaxStopDistanceM = 5.0;

bool IsBefore(SourcePosition const & lhs, SourcePosition const & rhs)
{
  return std::tie(lhs.m_line, lhs.m_segment, lhs.m_fraction) < std::tie(rhs.m_line, rhs.m_segment, rhs.m_fraction);
}

bool IsJoinedToNext(kml::MultiGeometry const & geometry, size_t line)
{
  auto const & lines = geometry.m_lines;
  ASSERT_LESS(line, lines.size(), ());
  ASSERT_GREATER(lines[line].size(), 1, ());
  if (line + 1 == lines.size())
    return false;
  ASSERT_GREATER(lines[line + 1].size(), 1, ());
  return lines[line].back().GetPoint().EqualDxDy(lines[line + 1].front().GetPoint(), mercator::kPointEqualityEps);
}

SourcePosition NormalizePosition(kml::MultiGeometry const & geometry, SourcePosition position)
{
  if (position.m_fraction < 1.0 - kEndpointFractionEps)
    return position;
  position.m_fraction = 0.0;
  ++position.m_segment;
  auto const & line = geometry.m_lines[position.m_line];
  if (position.m_segment + 1 < line.size())
    return position;
  if (IsJoinedToNext(geometry, position.m_line))
    return {position.m_line + 1, 0, 0.0};
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
  double m_distanceM;
};

std::optional<StopProjection> ProjectStop(kml::MultiGeometry const & geometry, m2::PointD const & point,
                                          SourcePosition const & bound, bool isSourceVertex, bool requireReturn = false)
{
  std::optional<StopProjection> best;
  ASSERT_LESS(bound.m_line, geometry.m_lines.size(), ());
  ASSERT_GREATER(geometry.m_lines[bound.m_line].size(), 1, ());
  ASSERT_LESS(bound.m_segment, geometry.m_lines[bound.m_line].size() - 1, ());
  // An off-vertex visit continues until the track leaves the stop's projection radius.
  auto const departureCenter = requireReturn && isSourceVertex ? PointAtPosition(geometry, bound).GetPoint() : point;
  double const departureRadiusM = isSourceVertex ? kDistinctStopVisitM : kMaxStopDistanceM;
  bool canProject = !requireReturn;
  for (size_t lineIndex = bound.m_line; lineIndex < geometry.m_lines.size(); ++lineIndex)
  {
    auto const & line = geometry.m_lines[lineIndex];
    size_t const firstSegment = lineIndex == bound.m_line ? bound.m_segment : 0;
    for (size_t segment = firstSegment; segment + 1 < line.size(); ++segment)
    {
      auto const & from = line[segment].GetPoint();
      auto const & to = line[segment + 1].GetPoint();
      double const minFraction = lineIndex == bound.m_line && segment == bound.m_segment ? bound.m_fraction : 0.0;
      double const fraction = std::max(minFraction, m2::SegmentProjectionFraction(from, to, point));
      auto const projected = m2::SegmentPointAt(from, to, fraction);
      if (!canProject)
      {
        auto const fromBound = m2::SegmentPointAt(from, to, minFraction);
        canProject = mercator::DistanceOnEarth(departureCenter, fromBound) > departureRadiusM;
      }
      double const distanceM = mercator::DistanceOnEarth(projected, point);
      if (canProject && distanceM <= kMaxStopDistanceM &&
          (!isSourceVertex || projected.EqualDxDy(point, mercator::kPointEqualityEps)) &&
          (!best || distanceM < best->m_distanceM))
      {
        best = StopProjection{NormalizePosition(geometry, {lineIndex, segment, fraction}), distanceM};
      }
      // Vertices identify exact source occurrences; noisy interpolated stops use the first nearby visit.
      if (best && (isSourceVertex || mercator::DistanceOnEarth(to, point) > kMaxStopDistanceM))
        return best;
      if (!canProject)
        canProject = mercator::DistanceOnEarth(departureCenter, to) > departureRadiusM;
    }
    if (best && !IsJoinedToNext(geometry, lineIndex))
      return best;
  }
  return best;
}
}  // namespace

TrackSourcePosition ResolveTrackEntryAt(Track const & track, double distanceFromBeginningM)
{
  auto const & lengths = track.GetCumulativeLengths();
  ASSERT(distanceFromBeginningM >= 0.0 && distanceFromBeginningM <= lengths.back().back(),
         (distanceFromBeginningM, lengths.back().back()));
  auto const line =
      std::ranges::lower_bound(lengths, distanceFromBeginningM, {}, [](auto const & l) { return l.back(); });
  // Search segment ends. At equal distances this keeps the first occurrence, including at a gap.
  auto const end = std::lower_bound(line->begin() + 1, line->end(), distanceFromBeginningM);
  double const start = *std::prev(end);
  double const length = *end - start;
  double const fraction = length > 0.0 ? (distanceFromBeginningM - start) / length : 0.0;
  auto const lineIndex = static_cast<size_t>(line - lengths.begin());
  auto const segment = static_cast<size_t>(end - line->begin() - 1);
  return NormalizePosition(track.GetData().m_geometry, {lineIndex, segment, fraction});
}

std::optional<TrackSourcePosition> ResolveTrackEntry(Track const & track, m2::PointD const & selectedPoint,
                                                     double distanceFromBeginningM)
{
  if (distanceFromBeginningM < 0.0)
    return std::nullopt;

  auto const & lengths = track.GetCumulativeLengths();
  auto const & geometry = track.GetData().m_geometry;
  auto const endsTooEarly = [&](double end) { return distanceFromBeginningM - end > kSelectionDistanceToleranceM; };
  auto const firstLine = std::ranges::partition_point(lengths, endsTooEarly, [](auto const & l) { return l.back(); });
  for (auto lineIt = firstLine; lineIt != lengths.end(); ++lineIt)
  {
    size_t const lineIndex = lineIt - lengths.begin();
    auto const & line = geometry.m_lines[lineIndex];
    auto const & lineLengths = *lineIt;
    auto const firstEnd = std::partition_point(lineLengths.begin() + 1, lineLengths.end(), endsTooEarly);
    for (auto end = firstEnd; end != lineLengths.end(); ++end)
    {
      double const start = *std::prev(end);
      if (start - distanceFromBeginningM > kSelectionDistanceToleranceM)
        return std::nullopt;
      size_t const segment = end - lineLengths.begin() - 1;
      auto const & from = line[segment].GetPoint();
      auto const & to = line[segment + 1].GetPoint();
      double const fraction = m2::SegmentProjectionFraction(from, to, selectedPoint);
      auto const projected = m2::SegmentPointAt(from, to, fraction);
      double const selectedDistance = std::lerp(start, *end, fraction);
      if (projected.EqualDxDy(selectedPoint, mercator::kPointEqualityEps) &&
          AlmostEqualAbs(distanceFromBeginningM, selectedDistance, kSelectionDistanceToleranceM))
      {
        return NormalizePosition(geometry, {lineIndex, segment, fraction});
      }
    }
  }
  return std::nullopt;
}

bool HasRemainingTrackGeometry(kml::MultiGeometry const & geometry, TrackSourcePosition const & entry)
{
  ASSERT_LESS(entry.m_line, geometry.m_lines.size(), ());
  ASSERT_GREATER(geometry.m_lines[entry.m_line].size(), 1, ());
  ASSERT_LESS(entry.m_segment, geometry.m_lines[entry.m_line].size() - 1, ());
  ASSERT(entry.m_fraction >= 0.0 && entry.m_fraction <= 1.0, (entry.m_fraction));
  auto const origin = PointAtPosition(geometry, entry).GetPoint();
  for (size_t line = entry.m_line;; ++line)
  {
    size_t const first = line == entry.m_line ? entry.m_segment + 1 : 0;
    for (size_t i = first; i < geometry.m_lines[line].size(); ++i)
      if (!geometry.m_lines[line][i].GetPoint().EqualDxDy(origin, mercator::kPointEqualityEps))
        return true;
    if (!IsJoinedToNext(geometry, line))
      return false;
  }
}

std::optional<routing::TrackFollowingData> BuildTrackFollowingData(kml::TrackData const & source,
                                                                   TrackSourcePosition const & entry)
{
  routing::TrackFollowingData result;
  auto const & geometry = source.m_geometry;
  if (!HasRemainingTrackGeometry(geometry, entry))
    return std::nullopt;

  size_t lastLine = entry.m_line;
  while (IsJoinedToNext(geometry, lastLine))
    ++lastLine;

  struct Event
  {
    SourcePosition m_position;
    PWA m_point;
    bool m_stop = false;
  };
  std::vector<Event> events;
  events.push_back({entry, PointAtPosition(geometry, entry)});

  auto const inRemainder = [&](SourcePosition const & position)
  { return position.m_line >= entry.m_line && position.m_line <= lastLine && !IsBefore(position, entry); };
  for (size_t lineIndex = entry.m_line; lineIndex <= lastLine; ++lineIndex)
  {
    auto const & line = geometry.m_lines[lineIndex];
    for (size_t pointIndex = 0; pointIndex < line.size(); ++pointIndex)
    {
      auto const position = pointIndex + 1 < line.size() ? SourcePosition{lineIndex, pointIndex, 0.0}
                                                         : SourcePosition{lineIndex, pointIndex - 1, 1.0};
      if (inRemainder(position))
        events.push_back({position, line[pointIndex]});
    }
  }

  auto const geometryEnd = events.size();
  // Preserve exact stops from saved routes and GPX, even when an earlier pass is nearby.
  std::unordered_map<m2::PointD, bool, m2::PointD::Hash> stopsAtVertices;
  stopsAtVertices.reserve(source.m_stops.size());
  for (auto const & stop : source.m_stops)
    stopsAtVertices.emplace(stop.m_point, false);
  if (!stopsAtVertices.empty())
    for (auto const & line : geometry.m_lines)
      for (auto const & point : line)
        if (auto const it = stopsAtVertices.find(point.GetPoint()); it != stopsAtVertices.end())
          it->second = true;

  SourcePosition previousStop;
  std::optional<m2::PointD> previousStopPoint;
  for (auto const & stop : source.m_stops)
  {
    bool const isSourceVertex = stopsAtVertices.at(stop.m_point);
    auto projected = ProjectStop(geometry, stop.m_point, previousStop, isSourceVertex);
    if (!projected)
    {
      LOG(LWARNING, ("Skipping a track stop without nearby source geometry", source.m_id, stop.m_name));
      continue;
    }
    // Stops carry coordinates and order. Prefer a later return after real intervening
    // geometry for repeated coordinates; retain the same occurrence for collocated duplicates
    // when no equally close later visit exists.
    if (previousStopPoint && previousStopPoint->EqualDxDy(stop.m_point, mercator::kPointEqualityEps))
    {
      if (auto const later = ProjectStop(geometry, stop.m_point, previousStop, isSourceVertex, true))
      {
        if (later->m_distanceM <= projected->m_distanceM + kStopProjectionToleranceM)
          projected = later;
      }
    }
    previousStop = projected->m_position;
    previousStopPoint = stop.m_point;
    if (!inRemainder(projected->m_position))
      continue;
    auto const anchor = PointAtPosition(geometry, projected->m_position);
    events.push_back({projected->m_position, anchor, true});
  }

  auto const bySourcePosition = [](Event const & lhs, Event const & rhs)
  { return IsBefore(lhs.m_position, rhs.m_position); };
  auto const stopsBegin = events.begin() + geometryEnd;
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
