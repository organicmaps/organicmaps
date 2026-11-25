#include "map/track_follow_data.hpp"

#include "map/track.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
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
// GPS fixes at the same trailhead can differ by metres. A short open section must still have distinct ends.
double constexpr kMaxLoopGapM = 10.0;
double constexpr kMaxLoopGapFraction = 0.1;

bool SamePosition(m2::PointD const & a, m2::PointD const & b)
{
  return mercator::NearestWorldCopy(a, b.x).EqualDxDy(b, mercator::kPointEqualityEps);
}

bool IsJoinedToNext(kml::MultiGeometry const & geometry, size_t line)
{
  auto const & lines = geometry.m_lines;
  ASSERT_LESS(line, lines.size(), ());
  ASSERT_GREATER(lines[line].size(), 1, ());
  if (line + 1 == lines.size())
    return false;
  ASSERT_GREATER(lines[line + 1].size(), 1, ());
  return SamePosition(lines[line].back().GetPoint(), lines[line + 1].front().GetPoint());
}

SourcePosition VertexPosition(kml::MultiGeometry const & geometry, size_t line, size_t vertex)
{
  if (vertex + 1 < geometry.m_lines[line].size())
    return {line, vertex, 0.0};
  if (IsJoinedToNext(geometry, line))
    return {line + 1, 0, 0.0};
  return {line, vertex - 1, 1.0};
}

SourcePosition NormalizePosition(kml::MultiGeometry const & geometry, SourcePosition const & position)
{
  if (position.m_fraction < 1.0 - kEndpointFractionEps)
    return position;
  return VertexPosition(geometry, position.m_line, position.m_segment + 1);
}

PWA PointAtPosition(kml::MultiGeometry const & geometry, SourcePosition const & position)
{
  auto const & line = geometry.m_lines[position.m_line];
  auto const & from = line[position.m_segment];
  auto const & to = line[position.m_segment + 1];
  return from.Interpolate(to, position.m_fraction);
}

// In a GPX route every intermediate point is a stop. Consecutive stops may share a compacted point.
std::optional<routing::TrackFollowingData> PrepareWaypointRoute(kml::TrackData const & source,
                                                                SourcePosition const & start,
                                                                SourcePosition const & end)
{
  auto const & geometry = source.m_geometry;
  auto const & line = geometry.m_lines.front();
  auto const & stops = source.m_stops;
  if (geometry.m_lines.size() != 1 || line.size() < 3 || stops.size() < line.size() - 2)
    return std::nullopt;

  routing::TrackFollowingData result;
  result.m_points.reserve(end.m_segment - start.m_segment + 2);
  result.m_points.push_back(PointAtPosition(geometry, start));
  auto stop = stops.begin();
  for (size_t i = 0; i < line.size(); ++i)
  {
    // Compaction keeps the first coordinate of each group.
    if (i != 0 && i + 1 < line.size() && (stop == stops.end() || !SamePosition(stop->m_point, line[i].GetPoint())))
      return std::nullopt;
    auto const first = stop;
    while (stop != stops.end() && kml::IsSameTrackPoint(stop->m_point, line[i].GetPoint()))
      ++stop;
    auto const position = VertexPosition(geometry, 0, i);
    if (position < start || position > end)
      continue;
    if (result.m_points.back() != line[i])
      result.m_points.push_back(line[i]);
    result.m_stopPointIndexes.insert(result.m_stopPointIndexes.end(), stop - first, result.m_points.size() - 1);
  }
  if (stop != stops.end())
    return std::nullopt;
  auto const finish = PointAtPosition(geometry, end);
  if (result.m_points.back() != finish)
    result.m_points.push_back(finish);
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
      if (!canProject)
      {
        auto const fromBound = mercator::Interpolate(from, to, minFraction);
        canProject = mercator::DistanceOnEarth(departureCenter, fromBound) > departureRadiusM;
      }
      if (canProject && isSourceVertex)
      {
        if (minFraction == 0.0 && from == point)
          return StopProjection{{lineIndex, segment, 0.0}, 0.0};
        if (to == point)
          return StopProjection{VertexPosition(geometry, lineIndex, segment + 1), 0.0};
      }
      else if (canProject)
      {
        double const fraction = std::max(minFraction, mercator::SegmentProjectionFraction(from, to, point));
        double const distanceM = mercator::DistanceOnEarth(mercator::Interpolate(from, to, fraction), point);
        if (distanceM <= kMaxStopDistanceM && (!best || distanceM < best->m_distanceM))
          best = StopProjection{NormalizePosition(geometry, {lineIndex, segment, fraction}), distanceM};
        if (best && mercator::DistanceOnEarth(to, point) > kMaxStopDistanceM)
          return best;
      }
      if (!canProject)
        canProject = mercator::DistanceOnEarth(departureCenter, to) > departureRadiusM;
    }
    if (best && !IsJoinedToNext(geometry, lineIndex))
      return best;
  }
  return best;
}
}  // namespace

double TrackSourcePosition::GetDistanceMeters(Track const & track) const
{
  auto const & lengths = track.GetCumulativeLengths()[m_line];
  return std::lerp(lengths[m_segment], lengths[m_segment + 1], m_fraction);
}

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
      double const fraction = mercator::SegmentProjectionFraction(from, to, selectedPoint);
      auto const projected = mercator::Interpolate(from, to, fraction);
      double const selectedDistance = std::lerp(start, *end, fraction);
      if (SamePosition(projected, selectedPoint) &&
          AlmostEqualAbs(distanceFromBeginningM, selectedDistance, kSelectionDistanceToleranceM))
      {
        return NormalizePosition(geometry, {lineIndex, segment, fraction});
      }
    }
  }
  return std::nullopt;
}

bool HasRemainingTrackGeometry(kml::MultiGeometry const & geometry, TrackSourcePosition const & entry, bool reverse)
{
  ASSERT_LESS(entry.m_line, geometry.m_lines.size(), ());
  ASSERT_GREATER(geometry.m_lines[entry.m_line].size(), 1, ());
  ASSERT_LESS(entry.m_segment, geometry.m_lines[entry.m_line].size() - 1, ());
  ASSERT(entry.m_fraction >= 0.0 && entry.m_fraction <= 1.0, (entry.m_fraction));
  auto const origin = PointAtPosition(geometry, entry).GetPoint();
  for (size_t line = entry.m_line;;)
  {
    auto const & points = geometry.m_lines[line];
    size_t const first = !reverse && line == entry.m_line ? entry.m_segment + 1 : 0;
    size_t const end = reverse && line == entry.m_line ? entry.m_segment + 1 : points.size();
    for (size_t i = first; i < end; ++i)
      if (!SamePosition(points[i].GetPoint(), origin))
        return true;
    if (reverse)
    {
      if (line == 0 || !IsJoinedToNext(geometry, line - 1))
        return false;
      --line;
    }
    else
    {
      if (!IsJoinedToNext(geometry, line))
        return false;
      ++line;
    }
  }
}

namespace
{
// Resolve stops in source order so repeated visits keep the same identity in either direction.
routing::TrackFollowingData PrepareTrackRange(kml::TrackData const & source, SourcePosition const & start,
                                              SourcePosition const & end)
{
  if (auto route = PrepareWaypointRoute(source, start, end))
    return std::move(*route);

  auto const & geometry = source.m_geometry;
  routing::TrackFollowingData result;
  size_t vertices = end.m_segment + 2;
  for (size_t line = start.m_line; line < end.m_line; ++line)
    vertices += geometry.m_lines[line].size();
  result.m_points.reserve(vertices - start.m_segment);

  auto const append = [&](PWA const & point)
  {
    if (result.m_points.empty() || result.m_points.back() != point)
      result.m_points.push_back(point);
  };
  size_t lineIndex = start.m_line;
  size_t vertex = start.m_segment + 1;
  append(PointAtPosition(geometry, start));
  auto const appendThrough = [&](size_t lastLineIndex, size_t lastVertex)
  {
    for (; lineIndex < lastLineIndex; ++lineIndex)
    {
      auto const & line = geometry.m_lines[lineIndex];
      for (; vertex < line.size(); ++vertex)
        append(line[vertex]);
      vertex = 0;
    }
    for (; vertex <= lastVertex; ++vertex)
      append(geometry.m_lines[lineIndex][vertex]);
  };

  std::unordered_map<m2::PointD, bool, m2::PointD::Hash> stopsAtVertices;
  for (auto const & stop : source.m_stops)
    stopsAtVertices.emplace(stop.m_point, false);
  if (!stopsAtVertices.empty())
    for (auto const & line : geometry.m_lines)
      for (auto const & point : line)
        if (auto const it = stopsAtVertices.find(point.GetPoint()); it != stopsAtVertices.end())
          it->second = true;

  SourcePosition previousStop;
  m2::PointD const * previousStopPoint = nullptr;
  for (auto const & stop : source.m_stops)
  {
    bool const isSourceVertex = stopsAtVertices.at(stop.m_point);
    auto projected = ProjectStop(geometry, stop.m_point, previousStop, isSourceVertex);
    if (!projected)
    {
      LOG(LWARNING, ("Skipping a track stop without nearby source geometry", source.m_id, stop.m_name));
      continue;
    }
    // Collocated stops may name the same visit or a later return to the same point.
    if (previousStopPoint && SamePosition(*previousStopPoint, stop.m_point))
    {
      auto const later = ProjectStop(geometry, stop.m_point, previousStop, isSourceVertex, true);
      if (later && later->m_distanceM <= projected->m_distanceM + kStopProjectionToleranceM)
        projected = later;
    }
    auto const position = projected->m_position;
    if (position > end)
      break;
    previousStop = position;
    previousStopPoint = &stop.m_point;
    if (position < start)
      continue;
    appendThrough(position.m_line, position.m_fraction == 1.0 ? position.m_segment + 1 : position.m_segment);
    append(PointAtPosition(geometry, position));
    result.m_stopPointIndexes.push_back(result.m_points.size() - 1);
  }
  appendThrough(end.m_line, end.m_segment);
  append(PointAtPosition(geometry, end));
  return result;
}

TrackSourcePosition GetTrackSectionEnd(kml::MultiGeometry const & geometry, size_t line, bool reverse)
{
  if (reverse)
  {
    while (line > 0 && IsJoinedToNext(geometry, line - 1))
      --line;
    return {line, 0, 0.0};
  }
  while (IsJoinedToNext(geometry, line))
    ++line;
  return {line, geometry.m_lines[line].size() - 2, 1.0};
}
}  // namespace

std::optional<TrackSourcePosition> GetReverseTrackEntry(Track const & track, TrackSourcePosition const & entry,
                                                        TrackSourcePosition const & destination)
{
  auto const & geometry = track.GetData().m_geometry;
  bool const currentlyReversed = destination < entry;
  auto const origin = GetTrackSectionEnd(geometry, entry.m_line, !currentlyReversed);
  double const gapM = mercator::DistanceOnEarth(PointAtPosition(geometry, origin).GetPoint(),
                                                PointAtPosition(geometry, destination).GetPoint());
  double const originM = origin.GetDistanceMeters(track);
  double const sectionLengthM = std::abs(destination.GetDistanceMeters(track) - originM);
  double const toleranceM = std::min(kMaxLoopGapM, kMaxLoopGapFraction * sectionLengthM);
  // Distance along the section distinguishes the trailhead from a later visit to the same place.
  if (gapM <= toleranceM && std::abs(entry.GetDistanceMeters(track) - originM) <= toleranceM)
    return destination;
  if (HasRemainingTrackGeometry(geometry, entry, !currentlyReversed))
    return entry;
  return std::nullopt;
}

std::optional<routing::TrackFollowingData> BuildTrackFollowingData(kml::TrackData const & source,
                                                                   TrackSourcePosition const & entry, bool reverse,
                                                                   TrackSourcePosition * destination)
{
  auto const & geometry = source.m_geometry;
  if (!HasRemainingTrackGeometry(geometry, entry, reverse))
    return std::nullopt;

  auto const finish = GetTrackSectionEnd(geometry, entry.m_line, reverse);
  if (destination)
    *destination = finish;
  if (!reverse)
    return PrepareTrackRange(source, entry, finish);

  auto result = PrepareTrackRange(source, finish, entry);
  std::ranges::reverse(result.m_points);
  std::ranges::reverse(result.m_stopPointIndexes);
  for (auto & index : result.m_stopPointIndexes)
    index = result.m_points.size() - 1 - index;
  return result;
}
