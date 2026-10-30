#include "routing/water_geometry.hpp"

#include "routing/base/astar_progress.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <tuple>
#include <utility>

namespace routing::water
{
namespace
{
using Point = m2::PointD;
using Interval = std::pair<double, double>;
double constexpr kInfinity = std::numeric_limits<double>::max();
size_t constexpr kMaxTransferVertices = 2048;
size_t constexpr kMaxVisibilityChecks = 4000000;
size_t constexpr kMaxVisibilityTriangleVisits = 64000000;
size_t constexpr kMaxWalkingCandidates = 4000000;
size_t constexpr kMaxWaterConnections = 4000000;

bool OnSide(m2::RectD const & bounds, Point const & point, size_t side)
{
  switch (side)
  {
  case 0: return point.x <= bounds.minX() + 1e-12;
  case 1: return point.x >= bounds.maxX() - 1e-12;
  case 2: return point.y <= bounds.minY() + 1e-12;
  default: return point.y >= bounds.maxY() - 1e-12;
  }
}

double BoundaryDistanceBound(Point const & a, Point const & b)
{
  return ms::kEarthRadiusMeters *
         math::DegToRad(std::abs(mercator::YToLat(a.y) - mercator::YToLat(b.y)) + std::abs(a.x - b.x));
}
bool Intersects(m2::RectD const & rect, Point const & a, Point const & b)
{
  double begin = 0, end = 1;
  auto const delta = b - a;
  for (auto const [p, q] : std::array<Interval, 4>{Interval{-delta.x, a.x - rect.minX()},
                                                   {delta.x, rect.maxX() - a.x},
                                                   {-delta.y, a.y - rect.minY()},
                                                   {delta.y, rect.maxY() - a.y}})
  {
    if (p == 0)
    {
      if (q < 0)
        return false;
      continue;
    }
    auto const t = q / p;
    if (p < 0)
      begin = std::max(begin, t);
    else
      end = std::min(end, t);
    if (begin > end)
      return false;
  }
  return true;
}

bool TouchesUnknown(Geometry const & geometry, Point const & a, Point const & b)
{
  return std::any_of(geometry.m_unknownAreas.begin(), geometry.m_unknownAreas.end(),
                     [&](auto const & rect) { return Intersects(rect, a, b); });
}

double Dot(Point const & a, Point const & b)
{
  return a.x * b.x + a.y * b.y;
}

Point Project(Point const & p, Point const & a, Point const & b)
{
  auto const d = b - a;
  auto const length = Dot(d, d);
  return length == 0.0 ? a : a + d * std::clamp(Dot(p - a, d) / length, 0.0, 1.0);
}

// Clip the parameter interval of a segment against all three triangle half-planes.
bool Clip(Point const & a, Point const & b, std::array<Point, 3> const & triangle, Interval & interval)
{
  auto const orientation = m2::robust::OrientedS(triangle[0], triangle[1], triangle[2]);
  if (orientation == 0.0)
    return false;
  auto const sign = orientation > 0.0 ? 1.0 : -1.0;
  interval = {0.0, 1.0};
  for (size_t i = 0; i < 3; ++i)
  {
    auto first = triangle[i];
    auto second = triangle[(i + 1) % 3];
    auto side = sign;
    // Adjacent triangles must calculate their shared edge using identical arithmetic.
    if (second < first)
    {
      std::swap(first, second);
      side = -side;
    }
    auto const initial = side * m2::robust::OrientedS(first, second, a);
    auto const terminal = side * m2::robust::OrientedS(first, second, b);
    auto const change = terminal - initial;
    if (change == 0.0)
    {
      if (initial < 0.0)
        return false;
      continue;
    }
    auto const t = -initial / change;
    if (change > 0.0)
      interval.first = std::max(interval.first, t);
    else
      interval.second = std::min(interval.second, t);
    if (interval.first > interval.second)
      return false;
  }
  return true;
}

std::vector<Interval> Coverage(Geometry const & geometry, Point const & a, Point const & b)
{
  std::vector<Interval> intervals;
  auto const visit = [&](std::array<Point, 3> const & triangle)
  {
    Interval interval;
    if (Clip(a, b, triangle, interval))
      intervals.push_back(interval);
  };
  if (geometry.m_triangleIndex)
  {
    // Tree rectangles use open boundaries. Include contacts before the exact
    // triangle predicate, including zero-width point and shoreline queries.
    m2::RectD bounds(a, b);
    bounds = {std::nextafter(bounds.minX(), -kInfinity), std::nextafter(bounds.minY(), -kInfinity),
              std::nextafter(bounds.maxX(), kInfinity), std::nextafter(bounds.maxY(), kInfinity)};
    geometry.m_triangleIndex->ForEachInRect(bounds, [&](size_t i) { visit(geometry.m_triangles[i]); });
  }
  else
    for (auto const & triangle : geometry.m_triangles)
      visit(triangle);
  std::sort(intervals.begin(), intervals.end());
  std::vector<Interval> result;
  for (auto const & interval : intervals)
    if (!result.empty() && interval.first <= result.back().second)
      result.back().second = std::max(result.back().second, interval.second);
    else
      result.push_back(interval);
  return result;
}

bool IsSurfacePoint(Geometry const & geometry, Point const & p)
{
  return !Coverage(geometry, p, p).empty();
}

bool CoversSegment(Geometry const & geometry, Point const & a, Point const & b, CancelledFn const & cancelled,
                   size_t & visits)
{
  if (!geometry.m_triangleIndex)
  {
    visits += geometry.m_triangles.size();
    if (visits > kMaxVisibilityTriangleVisits || cancelled())
      return false;
    auto const intervals = Coverage(geometry, a, b);
    return intervals.size() == 1 && intervals[0].first == 0.0 && intervals[0].second == 1.0;
  }
  // Walk successive covered intervals. A ray's bounding rectangle can contain
  // thousands of unrelated triangles, even when the ray crosses only a few.
  double covered = 0;
  for (size_t step = 0; step <= geometry.m_triangles.size(); ++step)
  {
    if (visits > kMaxVisibilityTriangleVisits || cancelled())
      return false;
    auto const point = covered == 0.0 ? a : a + (b - a) * covered;
    // Bound reconstruction error using the input magnitudes, including
    // cancellation near zero. Expanding by ULPs of point alone is insufficient.
    auto const errorX = 8 * std::numeric_limits<double>::epsilon() * (std::abs(a.x) + std::abs(b.x) + 1);
    auto const errorY = 8 * std::numeric_limits<double>::epsilon() * (std::abs(a.y) + std::abs(b.y) + 1);
    m2::RectD const bounds(point.x - errorX, point.y - errorY, point.x + errorX, point.y + errorY);
    double next = covered;
    geometry.m_triangleIndex->ForEachInRect(bounds, [&](size_t index)
    {
      if (++visits > kMaxVisibilityTriangleVisits || cancelled())
        return;
      Interval interval;
      if (Clip(a, b, geometry.m_triangles[index], interval) && interval.first <= covered)
        next = std::max(next, interval.second);
    });
    if (visits > kMaxVisibilityTriangleVisits || cancelled())
      return false;
    if (next == 1.0)
      return true;
    if (next <= covered)
      return false;
    covered = next;
  }
  return false;
}

bool MakePortal(Geometry const & geometry, Point const & a, Point const & b, double t, double interior, Point & point)
{
  // A parameter ULP may not change either geographic coordinate. Advance along
  // the source segment by a coordinate ULP toward the covered interval instead.
  auto const target = std::midpoint(t, interior);
  auto const delta = b - a;
  auto const position = [&](double parameter)
  { return parameter == 0.0 ? a : (parameter == 1.0 ? b : a + delta * parameter); };
  for (size_t attempt = 0; attempt < 16; ++attempt)
  {
    point = position(t);
    if (IsSurfacePoint(geometry, point))
      return true;
    auto const toward = position(target);
    double step = 0;
    if (delta.x != 0)
      step = std::max(step, std::abs((std::nextafter(point.x, toward.x) - point.x) / delta.x));
    if (delta.y != 0)
      step = std::max(step, std::abs((std::nextafter(point.y, toward.y) - point.y) / delta.y));
    auto const next = t + std::copysign(step, target - t);
    t = target > t ? std::min(next, target) : std::max(next, target);
  }
  point = position(t);
  if (IsSurfacePoint(geometry, point))
    return true;
  // Intersection parameters can accumulate more error than a few coordinate ULPs.
  // Approach the boundary from a certified interior point.
  auto inside = target;
  if (!IsSurfacePoint(geometry, position(inside)))
    return false;
  for (size_t attempt = 0; attempt < 64; ++attempt)
  {
    auto const middle = std::midpoint(t, inside);
    if (middle == t || middle == inside)
      break;
    if (IsSurfacePoint(geometry, position(middle)))
      inside = middle;
    else
      t = middle;
  }
  point = position(inside);
  return IsSurfacePoint(geometry, point);
}

struct Edge
{
  size_t m_target;
  double m_distance;
};

struct Graph
{
  explicit Graph(Geometry const & geometry) : m_geometry(geometry) {}
  Geometry const & m_geometry;
  std::vector<Point> m_points;
  std::vector<std::vector<Edge>> m_edges;
  std::vector<bool> m_lineEndpoints;
  std::vector<bool> m_unknownFrontier;
  std::vector<size_t> m_components;
  std::vector<uint8_t> m_ranks;
  std::vector<std::pair<size_t, size_t>> m_unknownEdges;
  std::vector<std::pair<size_t, size_t>> m_frontierEdges;

  size_t Add(Point const & p, bool lineEndpoint = false)
  {
    m_components.push_back(m_points.size());
    m_ranks.push_back(0);
    m_points.push_back(p);
    m_edges.emplace_back();
    m_lineEndpoints.push_back(lineEndpoint);
    m_unknownFrontier.push_back(TouchesUnknown(m_geometry, p, p));
    return m_points.size() - 1;
  }

  size_t Component(size_t node) const
  {
    while (m_components[node] != node)
      node = m_components[node];
    return node;
  }

  void Connect(size_t a, size_t b)
  {
    // Keep optimistic mesh connectivity separate from traversable edges.
    auto rootA = Component(a), rootB = Component(b);
    if (rootA != rootB)
    {
      if (m_ranks[rootA] < m_ranks[rootB])
        std::swap(rootA, rootB);
      m_components[rootB] = rootA;
      if (m_ranks[rootA] == m_ranks[rootB])
        ++m_ranks[rootA];
    }
    if (TouchesUnknown(m_geometry, m_points[a], m_points[b]))
    {
      m_unknownFrontier[a] = m_unknownFrontier[b] = true;
      m_unknownEdges.emplace_back(a, b);
      return;
    }
    auto const distance = mercator::DistanceOnEarth(m_points[a], m_points[b]);
    m_edges[a].push_back({b, distance});
    m_edges[b].push_back({a, distance});
  }
};

struct Search
{
  std::vector<double> m_distances;
  std::vector<size_t> m_previous;
  bool m_budgetExceeded = false;
  std::vector<double> m_lowerDistances;
};

Search Dijkstra(Graph const & graph, size_t source, CancelledFn const & cancelled)
{
  Search result;
  result.m_distances.assign(graph.m_points.size(), kInfinity);
  result.m_previous.assign(graph.m_points.size(), graph.m_points.size());
  using Item = std::pair<double, size_t>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
  result.m_distances[source] = 0.0;
  queue.emplace(0.0, source);
  while (!queue.empty() && !cancelled())
  {
    auto const [distance, node] = queue.top();
    queue.pop();
    if (distance != result.m_distances[node])
      continue;
    for (auto const & edge : graph.m_edges[node])
    {
      auto const candidate = distance + edge.m_distance;
      if (candidate >= result.m_distances[edge.m_target])
        continue;
      result.m_distances[edge.m_target] = candidate;
      result.m_previous[edge.m_target] = node;
      queue.emplace(candidate, edge.m_target);
    }
  }
  return result;
}

Search SearchWater(Graph & graph, size_t target, std::vector<size_t> const & visibility, Search const & component,
                   CancelledFn const & isCancelled, ProgressFn const & progress, double frontierMargin = 0)
{
  // The delegate's cancellation check locks a mutex and reads the clock.
  // Poll periodically while keeping the inner geometric predicates cheap.
  size_t cancellationChecks = 0;
  bool stopped = false;
  auto const cancelled = [&]
  {
    if ((cancellationChecks++ % 256) == 0)
      stopped = isCancelled();
    return stopped;
  };
  CancelledFn const segmentCancelled = cancelled;
  Search result;
  result.m_distances = component.m_distances;
  result.m_previous = component.m_previous;
  auto const source = target == 1 ? 0 : 1;
  // Visibility edges stay inside the source mesh component. Searching them
  // cannot reach a goal in another component; retain the valid sparse costs.
  if (graph.Component(source) != graph.Component(target))
  {
    result.m_lowerDistances = result.m_distances;
    return result;
  }
  std::vector<bool> visible(graph.m_points.size()), settled(graph.m_points.size());
  for (auto node : visibility)
    visible[node] = true;
  std::vector<double> estimates;
  std::vector<ms::LatLon> coordinates;
  coordinates.reserve(graph.m_points.size());
  for (auto const & point : graph.m_points)
    coordinates.push_back(mercator::ToLatLon(point));
  estimates.reserve(coordinates.size());
  for (auto const & coordinate : coordinates)
    estimates.push_back(ms::DistanceOnEarth(coordinate, coordinates[target]));
  // Every vertex of a shorter route must lie inside this distance ellipse.
  // Apply this bound once before considering pairs of visibility vertices.
  std::vector<size_t> candidates;
  auto const sourceComponent = graph.Component(source);
  for (auto const node : visibility)
  {
    if (graph.Component(node) != sourceComponent)
      continue;
    if (result.m_distances[target] == kInfinity ||
        ms::DistanceOnEarth(coordinates[source], coordinates[node]) + estimates[node] <
            result.m_distances[target] + frontierMargin)
      candidates.push_back(node);
  }
  // A certified mesh path is already usable. Avoid a quadratic optimization
  // exceeding the visibility budget; return it with shortest-path qualification.
  if (result.m_distances[target] != kInfinity && !candidates.empty() &&
      candidates.size() > kMaxVisibilityChecks / candidates.size())
  {
    result.m_budgetExceeded = true;
    return result;
  }
  using Item = std::pair<double, size_t>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
  for (size_t node = 0; node < graph.m_points.size(); ++node)
    if (result.m_distances[node] != kInfinity)
      queue.emplace(result.m_distances[node] + estimates[node], node);
  AStarSubProgress searchProgress(coordinates[source], coordinates[target], 1.0);
  size_t settledCount = 0;
  size_t checks = 0;
  size_t triangleVisits = 0;
  while (!queue.empty() && !cancelled())
  {
    auto const [score, node] = queue.top();
    queue.pop();
    if (settled[node] || score > result.m_distances[node] + estimates[node])
      continue;
    if (result.m_distances[target] != kInfinity && score > result.m_distances[target] + frontierMargin)
      break;
    settled[node] = true;
    if (progress && ((settledCount++ % 32) == 0 || node == target))
      progress(searchProgress.UpdateProgress(coordinates[node], coordinates[target]));
    if (node == target && frontierMargin == 0)
      break;
    auto const relax = [&](size_t other, double distance)
    {
      auto const candidate = result.m_distances[node] + distance;
      if (settled[other] || candidate >= result.m_distances[other])
        return;
      result.m_distances[other] = candidate;
      result.m_previous[other] = node;
      queue.emplace(candidate + estimates[other], other);
    };
    for (auto const & edge : graph.m_edges[node])
      relax(edge.m_target, edge.m_distance);
    if (!visible[node])
      continue;
    for (auto const other : candidates)
    {
      if (cancelled())
        return result;
      if (settled[other])
        continue;
      auto const distance = ms::DistanceOnEarth(coordinates[node], coordinates[other]);
      if (result.m_distances[node] + distance >= result.m_distances[other])
        continue;
      if (result.m_distances[target] != kInfinity &&
          result.m_distances[node] + distance + estimates[other] >= result.m_distances[target] + frontierMargin)
        continue;
      if (++checks > kMaxVisibilityChecks)
      {
        result.m_budgetExceeded = true;
        return result;
      }
      if (!CoversSegment(graph.m_geometry, graph.m_points[node], graph.m_points[other], segmentCancelled,
                         triangleVisits))
      {
        if (triangleVisits > kMaxVisibilityTriangleVisits)
        {
          result.m_budgetExceeded = true;
          return result;
        }
        continue;
      }
      if (TouchesUnknown(graph.m_geometry, graph.m_points[node], graph.m_points[other]))
      {
        graph.m_unknownFrontier[node] = graph.m_unknownFrontier[other] = true;
        continue;
      }
      relax(other, distance);
    }
  }
  // Unsettled goals have g + h at least the search threshold. Settled goals
  // have exact costs in the visibility graph.
  result.m_lowerDistances.resize(graph.m_points.size());
  for (size_t i = 0; i < graph.m_points.size(); ++i)
    result.m_lowerDistances[i] = settled[i] || result.m_distances[target] == kInfinity
                                   ? result.m_distances[i]
                                   : std::max(0.0, result.m_distances[target] + frontierMargin - estimates[i]);
  return result;
}

void Append(Path & path, Point const & point, bool land)
{
  if (!path.m_points.empty())
  {
    if (path.m_points.back() == point)
      return;
    auto const distance = mercator::DistanceOnEarth(path.m_points.back(), point);
    (land ? path.m_landDistance : path.m_waterDistance) += distance;
    path.m_landGaps.push_back(land);
  }
  path.m_points.push_back(point);
}

std::vector<size_t> Trace(Search const & search, size_t node)
{
  std::vector<size_t> result;
  while (node < search.m_previous.size())
  {
    result.push_back(node);
    node = search.m_previous[node];
  }
  return result;
}

struct Selection
{
  Point m_point;
  size_t m_line = std::numeric_limits<size_t>::max();
  size_t m_segment = 0;
  double m_distance = kInfinity;
};

Selection Select(Geometry const & geometry, Point const & point, bool knownOnly = false)
{
  if (IsSurfacePoint(geometry, point))
    return {point, std::numeric_limits<size_t>::max(), 0, 0.0};
  Selection result;
  auto const consider =
      [&](Point const & a, Point const & b, size_t line, size_t segment, Point const * interior = nullptr)
  {
    auto projection = Project(point, a, b);
    auto distance = mercator::DistanceOnEarth(point, projection);
    if (distance >= result.m_distance)
      return;
    // A rounded shoreline projection can lie just outside the triangle and
    // become an isolated graph vertex. Move toward certified water, not land.
    if (interior && !IsSurfacePoint(geometry, projection))
    {
      Point covered;
      if (!MakePortal(geometry, projection, *interior, 0.0, 1.0, covered))
        return;
      projection = covered;
      distance = mercator::DistanceOnEarth(point, projection);
    }
    if (distance < result.m_distance && (!knownOnly || !TouchesUnknown(geometry, point, projection)))
      result = {projection, line, segment, distance};
  };
  for (size_t i = 0; i < geometry.m_lines.size(); ++i)
  {
    auto const & line = geometry.m_lines[i];
    for (size_t j = 1; j < line.size(); ++j)
      consider(line[j - 1].m_point, line[j].m_point, i, j);
  }
  for (auto const & triangle : geometry.m_triangles)
  {
    auto const interior = (triangle[0] + triangle[1] + triangle[2]) / 3.0;
    for (size_t i = 0; i < 3; ++i)
      consider(triangle[i], triangle[(i + 1) % 3], std::numeric_limits<size_t>::max(), 0, &interior);
  }
  return result;
}

double ExitDistance(m2::RectD const & bounds, Point const & point)
{
  if (!bounds.IsPointInside(point))
    return 0;
  auto const latitude = mercator::YToLat(point.y);
  double result = std::min(latitude - mercator::YToLat(bounds.minY()), mercator::YToLat(bounds.maxY()) - latitude);
  result = math::DegToRad(result);
  for (auto const longitude : {bounds.minX(), bounds.maxX()})
    result = std::min(result, std::asin(std::abs(std::cos(math::DegToRad(latitude)) *
                                                 std::sin(math::DegToRad(longitude - point.x)))));
  return result * ms::kEarthRadiusMeters;
}

bool OutsideCouldShorten(Geometry const & geometry, Graph const & graph, Search const & search, size_t source,
                         Point const & to, double distance, CancelledFn const & cancelled)
{
  auto const & bounds = *geometry.m_bounds;
  auto const check = [&](auto && self, Point const & a, Point const & b, double lowerA, double lowerB,
                         size_t depth) -> bool
  {
    if (cancelled())
      return true;
    auto const middle = (a + b) * 0.5;
    double const radius = std::max(BoundaryDistanceBound(a, middle), BoundaryDistanceBound(b, middle));
    double const lower =
        std::max({0.0, lowerA - BoundaryDistanceBound(a, middle), lowerB - BoundaryDistanceBound(b, middle)});
    double const sum = lower + mercator::DistanceOnEarth(middle, to);
    if (sum - 2 * radius >= distance + 0.01)
      return false;
    if (sum < distance || depth == 12)
      return true;
    return self(self, a, middle, lowerA, lower, depth + 1) || self(self, middle, b, lower, lowerB, depth + 1);
  };
  auto const component = graph.Component(source);
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    if (graph.Component(i) != component)
      continue;
    auto const & point = graph.m_points[i];
    for (size_t side = 0; side < 4; ++side)
      if (OnSide(bounds, point, side) &&
          check(check, point, point, search.m_lowerDistances[i], search.m_lowerDistances[i], 0))
        return true;
  }
  for (auto const & [a, b] : graph.m_frontierEdges)
  {
    if (graph.Component(a) != component)
      continue;
    // Water costs are Lipschitz only along traversable boundary intervals.
    bool const unknown = TouchesUnknown(geometry, graph.m_points[a], graph.m_points[b]);
    double const lowerA =
        unknown ? mercator::DistanceOnEarth(graph.m_points[source], graph.m_points[a]) : search.m_lowerDistances[a];
    double const lowerB =
        unknown ? mercator::DistanceOnEarth(graph.m_points[source], graph.m_points[b]) : search.m_lowerDistances[b];
    if (check(check, graph.m_points[a], graph.m_points[b], lowerA, lowerB, 0))
      return true;
  }
  return false;
}
bool ReachesFrontier(Geometry const & geometry, Graph const & graph, size_t source, bool includeUnknown = true)
{
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    if (graph.Component(i) != graph.Component(source))
      continue;
    if (includeUnknown && graph.m_unknownFrontier[i])
      return true;
    if (!geometry.m_bounds)
      continue;
    auto const & bounds = *geometry.m_bounds;
    auto const & p = graph.m_points[i];
    if (p.x <= bounds.minX() + 1e-12 || p.x >= bounds.maxX() - 1e-12 || p.y <= bounds.minY() + 1e-12 ||
        p.y >= bounds.maxY() - 1e-12)
      return true;
  }
  return false;
}

// A spherical lower bound to any point in the rectangle. Latitude separation
// and distance to its bounding meridians are each lower bounds independently.
double DistanceToUnknown(m2::RectD const & rect, Point const & point)
{
  auto const latitude = mercator::YToLat(point.y);
  auto const low = mercator::YToLat(rect.minY());
  auto const high = mercator::YToLat(rect.maxY());
  double result = math::DegToRad(std::max({low - latitude, latitude - high, 0.0}));
  if (point.x < rect.minX() || point.x > rect.maxX())
  {
    auto const meridian = [&](double longitude)
    { return std::asin(std::abs(std::cos(math::DegToRad(latitude)) * std::sin(math::DegToRad(longitude - point.x)))); };
    result = std::max(result, std::min(meridian(rect.minX()), meridian(rect.maxX())));
  }
  return result * ms::kEarthRadiusMeters;
}

bool UnknownCouldShorten(Geometry const & geometry, Point const & from, Point const & to, double distance)
{
  return std::any_of(geometry.m_unknownAreas.begin(), geometry.m_unknownAreas.end(), [&](auto const & rect)
  { return DistanceToUnknown(rect, from) + DistanceToUnknown(rect, to) <= distance; });
}

// Water is searched first. This sparse second graph connects stored river ends
// and terminal walking access, allowing several portages without scanning every
// endpoint pair or changing the pedestrian router.
ResultCode FindWalkingPath(Graph const & graph, std::set<Point> const & shore, CancelledFn const & cancelled,
                           Path & path, ProgressFn const & progress, WalkAllowedFn const & walkAllowed)
{
  auto const count = graph.m_points.size();
  std::vector<size_t> components(count, count), stack;
  for (size_t root = 0; root < count; ++root)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    if (components[root] != count)
      continue;
    components[root] = root;
    stack.push_back(root);
    while (!stack.empty())
    {
      auto const node = stack.back();
      stack.pop_back();
      for (auto const & edge : graph.m_edges[node])
        if (components[edge.m_target] == count)
        {
          components[edge.m_target] = root;
          stack.push_back(edge.m_target);
        }
    }
  }
  std::vector<size_t> endpoints{0, 1};
  m4::Tree<size_t> index;
  for (size_t i = 2; i < count; ++i)
  {
    bool surfaceEnd = graph.m_geometry.m_hasSourceTopology && shore.contains(graph.m_points[i]);
    if (surfaceEnd && graph.m_geometry.m_bounds)
      for (size_t side = 0; side < 4; ++side)
        surfaceEnd = surfaceEnd && !OnSide(*graph.m_geometry.m_bounds, graph.m_points[i], side);
    if ((graph.m_lineEndpoints[i] || surfaceEnd) &&
        !TouchesUnknown(graph.m_geometry, graph.m_points[i], graph.m_points[i]))
      endpoints.push_back(i);
  }
  for (auto const node : endpoints)
    index.Add(node, m2::RectD(graph.m_points[node], graph.m_points[node]));
  index.Optimize();
  struct WalkEdge
  {
    size_t m_to;
    double m_distance;
    bool m_enabled = true;
    bool m_checked = false;
  };
  std::vector<std::vector<WalkEdge>> walking(count);
  std::set<std::pair<size_t, size_t>> links;
  size_t constexpr kMaxLinks = 131072;
  size_t constexpr kNeighbors = 16;
  size_t candidateChecks = 0;
  for (size_t i = 0; i < endpoints.size(); ++i)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    if (progress)
      progress(0.7 + 0.15 * i / endpoints.size());
    auto const from = endpoints[i];
    std::vector<std::pair<double, size_t>> neighbors;
    for (double const radius : {500.0, 2000.0, 5000.0, 20000.0, 80000.0, 320000.0, 1280000.0})
    {
      neighbors.clear();
      index.ForEachInRect(mercator::RectByCenterXYAndSizeInMeters(graph.m_points[from], radius), [&](size_t to)
      {
        if (++candidateChecks > kMaxWalkingCandidates)
          return;
        if (from == to || components[from] == components[to] || (from < 2 && to < 2) ||
            graph.m_points[from] == graph.m_points[to])
          return;
        auto const distance = mercator::DistanceOnEarth(graph.m_points[from], graph.m_points[to]);
        if (distance > radius || ((from < 2 || to < 2) && distance > 5000.0))
          return;
        neighbors.emplace_back(distance, to);
      });
      if (candidateChecks > kMaxWalkingCandidates)
        return ResultCode::Unavailable;
      std::sort(neighbors.begin(), neighbors.end());
      // Several ends per component allow retrying an inaccessible landing.
      std::map<size_t, size_t> perComponent;
      std::vector<std::pair<double, size_t>> selected;
      for (auto const & neighbor : neighbors)
        if (perComponent[components[neighbor.second]]++ < 4)
        {
          selected.push_back(neighbor);
          if (selected.size() == kNeighbors)
            break;
        }
      neighbors = std::move(selected);
      if (neighbors.size() == kNeighbors || (radius >= 5000.0 && (!neighbors.empty() || from < 2)))
        break;
    }
    for (auto const & [distance, to] : neighbors)
    {
      auto const pair = std::minmax(from, to);
      if (links.contains(pair) || TouchesUnknown(graph.m_geometry, graph.m_points[from], graph.m_points[to]))
        continue;
      if (links.size() == kMaxLinks)
        return ResultCode::Unavailable;
      links.insert(pair);
      walking[from].push_back({to, distance});
      walking[to].push_back({from, distance});
    }
  }
  // Search river endpoints rather than duplicating the full mesh for each
  // walking state. Water distances inside a component are calculated lazily.
  // Alternating water/walk states prevent replacing a paddle with chained walks.
  using Cost = std::tuple<size_t, double, double>;
  Cost const infinity{std::numeric_limits<size_t>::max(), kInfinity, kInfinity};
  auto const states = endpoints.size() * 4;
  std::vector<size_t> positions(count, endpoints.size());
  for (size_t i = 0; i < endpoints.size(); ++i)
    positions[endpoints[i]] = i;
  std::vector<std::vector<std::pair<size_t, double>>> water(endpoints.size());
  std::vector<bool> waterReady(endpoints.size(), false);
  size_t waterLinks = 0;
  struct Previous
  {
    size_t m_state;
    size_t m_walk;
  };
  size_t constexpr kMaxRejectedWalkingChecks = 32;
  size_t rejected = 0;
  for (;;)
  {
    std::vector<Cost> distances(states, infinity);
    std::vector<Previous> previous(states, {states, count});
    using Item = std::pair<Cost, size_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
    for (size_t const state : {size_t{0}, size_t{1}})
    {
      distances[state] = {0, 0.0, 0.0};
      queue.emplace(distances[state], state);
    }
    size_t goal = states;
    while (!queue.empty())
    {
      if (cancelled())
        return ResultCode::Cancelled;
      auto const [cost, state] = queue.top();
      queue.pop();
      if (cost != distances[state])
        continue;
      auto const endpoint = state / 4;
      auto const node = endpoints[endpoint];
      bool const hasWater = (state & 2) != 0;
      bool const paddleNext = (state & 1) != 0;
      if (node == 1 && hasWater)
      {
        goal = state;
        break;
      }
      auto const relax = [&](size_t to, double distance, size_t walk)
      {
        bool const land = walk != count;
        auto const nextState = positions[to] * 4 + ((hasWater || (!land && distance > 0.0)) ? 2 : 0) + (land ? 1 : 0);
        Cost const next{std::get<0>(cost) + (land && (node < 2 || to < 2)), std::get<1>(cost) + (land ? distance : 0.0),
                        std::get<2>(cost) + distance};
        if (next < distances[nextState])
        {
          distances[nextState] = next;
          previous[nextState] = {state, walk};
          queue.emplace(next, nextState);
        }
      };
      if (paddleNext)
      {
        if (!waterReady[endpoint])
        {
          auto const reached = Dijkstra(graph, node, cancelled);
          if (cancelled())
            return ResultCode::Cancelled;
          for (size_t j = 0; j < endpoints.size(); ++j)
            if (j != endpoint && reached.m_distances[endpoints[j]] != kInfinity)
            {
              if (++waterLinks > kMaxWaterConnections)
                return ResultCode::Unavailable;
              water[endpoint].emplace_back(endpoints[j], reached.m_distances[endpoints[j]]);
            }
          waterReady[endpoint] = true;
        }
        for (auto const & [to, distance] : water[endpoint])
          relax(to, distance, count);
      }
      else
        for (size_t j = 0; j < walking[node].size(); ++j)
          if (walking[node][j].m_enabled)
            relax(walking[node][j].m_to, walking[node][j].m_distance, j);
    }
    if (goal == states)
      return ResultCode::Unavailable;
    std::vector<size_t> nodes;
    for (auto state = goal; state != states; state = previous[state].m_state)
      nodes.push_back(state);
    std::reverse(nodes.begin(), nodes.end());
    bool retry = false;
    for (size_t i = 1; i < nodes.size(); ++i)
    {
      auto const to = endpoints[nodes[i] / 4];
      auto const from = endpoints[nodes[i - 1] / 4];
      auto const walk = previous[nodes[i]].m_walk;
      if (walk == count || walking[from][walk].m_checked || !walkAllowed)
        continue;
      auto & edge = walking[from][walk];
      edge.m_checked = true;
      edge.m_enabled = walkAllowed(graph.m_points[from], graph.m_points[to]);
      if (cancelled())
        return ResultCode::Cancelled;
      if (!edge.m_enabled)
      {
        if (++rejected == kMaxRejectedWalkingChecks)
          return ResultCode::Unavailable;
        retry = true;
        break;
      }
    }
    if (retry)
      continue;
    path.m_shortestConfirmed = false;
    Append(path, graph.m_points.front(), false);
    for (size_t i = 1; i < nodes.size(); ++i)
    {
      auto const to = endpoints[nodes[i] / 4];
      auto const from = endpoints[nodes[i - 1] / 4];
      if (previous[nodes[i]].m_walk != count)
        Append(path, graph.m_points[to], true);
      else
      {
        auto const reached = Dijkstra(graph, from, cancelled);
        auto const waterNodes = Trace(reached, to);
        if (cancelled())
        {
          path = {};
          return ResultCode::Cancelled;
        }
        for (auto it = waterNodes.rbegin(); it != waterNodes.rend(); ++it)
          Append(path, graph.m_points[*it], false);
      }
    }
    if (progress)
      progress(1.0);
    return ResultCode::Success;
  }
}
}  // namespace

std::optional<Point> FindApproachPoint(Geometry const & geometry, Point const & start, CancelledFn const & cancelled)
{
  if (!geometry.m_complete || cancelled() || TouchesUnknown(geometry, start, start))
    return {};
  auto const selected = Select(geometry, start, true);
  // Keep approaches local and do not select across missing coverage.
  if (selected.m_distance > 1000.0 || TouchesUnknown(geometry, selected.m_point, selected.m_point) ||
      (geometry.m_bounds && selected.m_distance >= ExitDistance(*geometry.m_bounds, start)))
    return {};
  return cancelled() ? std::nullopt : std::optional<Point>(selected.m_point);
}
ResultCode FindPath(Geometry const & geometry, Point const & start, Point const & finish, bool projectStart,
                    CancelledFn const & cancelled, Path & path, ProgressFn const & progress,
                    bool allowUnconfirmedTransfer, WalkAllowedFn const & walkAllowed)
{
  path = {};
  auto const report = [&](double value)
  {
    if (progress)
      progress(value);
  };
  if (cancelled())
    return ResultCode::Cancelled;
  if (!geometry.m_complete)
    return ResultCode::Unavailable;

  size_t count = 2;
  if (geometry.m_triangles.size() > kMaxTriangles || geometry.m_lines.size() > kMaxGraphVertices)
    return ResultCode::Unavailable;
  for (auto const & line : geometry.m_lines)
  {
    if (line.size() > kMaxGraphVertices - std::min(count, kMaxGraphVertices))
      return ResultCode::Unavailable;
    count += line.size();
  }
  if (count > kMaxGraphVertices)
    return ResultCode::Unavailable;
  if ((!projectStart && TouchesUnknown(geometry, start, start)) || TouchesUnknown(geometry, finish, finish))
    return ResultCode::Unavailable;

  auto const from = Select(geometry, start);
  auto const to = Select(geometry, finish);
  // Explicit selections must actually be on the supplied geometry. Only the implicit GPS start projects.
  if (from.m_distance == kInfinity || to.m_distance == kInfinity)
    return ResultCode::Unavailable;
  // Permit only the sub-decimetre quantization error of a selected MWM line.
  if ((!projectStart && from.m_distance > 0.1) || to.m_distance > 0.1)
    return ResultCode::OutsideWater;
  if (TouchesUnknown(geometry, from.m_point, from.m_point) || TouchesUnknown(geometry, to.m_point, to.m_point))
    return ResultCode::Unavailable;
  if (geometry.m_bounds &&
      (ExitDistance(*geometry.m_bounds, from.m_point) == 0 || ExitDistance(*geometry.m_bounds, to.m_point) == 0 ||
       (projectStart && from.m_distance >= ExitDistance(*geometry.m_bounds, start))))
    return ResultCode::Unavailable;
  if (projectStart && std::any_of(geometry.m_unknownAreas.begin(), geometry.m_unknownAreas.end(),
                                  [&](auto const & rect) { return DistanceToUnknown(rect, start) <= from.m_distance; }))
    return ResultCode::Unavailable;

  // Point eligibility needs only certified membership, not the visibility graph.
  if (from.m_point == to.m_point)
  {
    path.m_points.push_back(from.m_point);
    report(1.0);
    return ResultCode::Success;
  }
  size_t triangleVisits = 0;
  if (CoversSegment(geometry, from.m_point, to.m_point, cancelled, triangleVisits) &&
      !TouchesUnknown(geometry, from.m_point, to.m_point))
  {
    Append(path, from.m_point, false);
    Append(path, to.m_point, false);
    report(1.0);
    return ResultCode::Success;
  }
  if (cancelled())
    return ResultCode::Cancelled;
  if (triangleVisits > kMaxVisibilityTriangleVisits)
    return ResultCode::Unavailable;

  Graph graph(geometry);
  graph.Add(from.m_point);
  graph.Add(to.m_point);
  std::map<uint64_t, size_t> sourceNodes;
  std::set<size_t> surfacePortals{0, 1};
  for (size_t i = 0; i < geometry.m_lines.size(); ++i)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    report(0.1 * i / geometry.m_lines.size());
    auto const & line = geometry.m_lines[i];
    std::vector<size_t> nodes;
    for (size_t vertexIndex = 0; vertexIndex < line.size(); ++vertexIndex)
    {
      auto const & vertex = line[vertexIndex];
      auto const id =
          graph.Add(vertex.m_point, vertex.m_mayTransfer && (vertexIndex == 0 || vertexIndex + 1 == line.size()));
      nodes.push_back(id);
      if (vertex.m_nodeId != 0)
      {
        auto const [it, inserted] = sourceNodes.emplace(vertex.m_nodeId, id);
        if (!inserted)
        {
          if (graph.m_points[it->second] != vertex.m_point)
            return ResultCode::Unavailable;
          graph.Connect(it->second, id);
        }
      }
    }
    for (size_t j = 1; j < nodes.size(); ++j)
    {
      std::vector<size_t> split = {nodes[j - 1], nodes[j]};
      if (from.m_line == i && from.m_segment == j)
        split.push_back(0);
      if (to.m_line == i && to.m_segment == j)
        split.push_back(1);
      auto const origin = line[j - 1].m_point;
      for (auto const & interval : Coverage(geometry, origin, line[j].m_point))
      {
        // The existing vertices already connect a segment wholly inside water.
        if (interval.first == 0 && interval.second == 1)
          continue;
        Point entrance, exit;
        if (!MakePortal(geometry, origin, line[j].m_point, interval.first, interval.second, entrance) ||
            !MakePortal(geometry, origin, line[j].m_point, interval.second, interval.first, exit))
          return ResultCode::Unavailable;
        auto const entranceNode = graph.Add(entrance);
        auto const exitNode = graph.Add(exit);
        split.push_back(entranceNode);
        split.push_back(exitNode);
        surfacePortals.insert(entranceNode);
        surfacePortals.insert(exitNode);
      }
      if (graph.m_points.size() > kMaxGraphVertices)
        return ResultCode::Unavailable;
      std::sort(split.begin(), split.end(), [&](size_t a, size_t b)
      { return (graph.m_points[a] - origin).SquaredLength() < (graph.m_points[b] - origin).SquaredLength(); });
      for (size_t k = 1; k < split.size(); ++k)
        graph.Connect(split[k - 1], split[k]);
    }
  }
  auto const lineVertices = graph.m_points.size();
  std::map<Point, size_t> surfaceVertices;
  struct SurfaceEdge
  {
    size_t m_count = 0;
    Point m_from, m_to;
  };
  std::map<std::pair<Point, Point>, SurfaceEdge> surfaceEdges;
  size_t triangleCount = 0;
  for (auto const & triangle : geometry.m_triangles)
  {
    if ((triangleCount % 128) == 0)
    {
      if (cancelled())
        return ResultCode::Cancelled;
      report(0.1 + 0.1 * triangleCount / geometry.m_triangles.size());
    }
    ++triangleCount;
    m2::RectD bounds;
    for (auto const & point : triangle)
      bounds.Add(point);
    bool const unknown = std::any_of(geometry.m_unknownAreas.begin(), geometry.m_unknownAreas.end(),
                                     [&](auto const & rect) { return rect.IsIntersect(bounds); });
    std::array<size_t, 3> nodes;
    for (size_t i = 0; i < triangle.size(); ++i)
    {
      auto const & point = triangle[i];
      auto const [it, inserted] = surfaceVertices.emplace(point, graph.m_points.size());
      if (inserted)
      {
        if (graph.m_points.size() >= kMaxGraphVertices)
          return ResultCode::Unavailable;
        graph.Add(point);
      }
      if (unknown)
        graph.m_unknownFrontier[it->second] = true;
      nodes[i] = it->second;
    }
    for (size_t i = 0; i < triangle.size(); ++i)
    {
      auto const j = (i + 1) % triangle.size();
      auto a = triangle[i], b = triangle[j];
      if (b < a)
        std::swap(a, b);
      auto & edge = surfaceEdges[{a, b}];
      ++edge.m_count;
      bool const ccw = m2::robust::OrientedS(triangle[0], triangle[1], triangle[2]) > 0;
      edge.m_from = triangle[ccw ? i : j];
      edge.m_to = triangle[ccw ? j : i];
      graph.Connect(nodes[i], nodes[j]);
    }
  }

  // Attach checkpoints, river portals and exact surface contacts before the
  // lazy visibility search. This also preserves component/frontier reachability.
  size_t const meshVertices = graph.m_points.size();
  for (size_t i = 0; i < meshVertices; ++i)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    report(0.2 + 0.05 * i / meshVertices);
    auto const & point = graph.m_points[i];
    auto const attach = [&](std::array<Point, 3> const & triangle)
    {
      Interval interval;
      if (!Clip(point, point, triangle, interval))
        return;
      // Adjacent surface pieces may meet at a vertex inside another triangle's
      // edge. Stitch that exact covered contact before pruning visibility pairs
      // by mesh component; coordinate proximity alone never creates a join.
      if (i >= lineVertices && std::find(triangle.begin(), triangle.end(), point) != triangle.end())
        return;
      for (auto const & vertex : triangle)
        graph.Connect(i, surfaceVertices.at(vertex));
    };
    if (geometry.m_triangleIndex)
    {
      m2::RectD const bounds(std::nextafter(point.x, -kInfinity), std::nextafter(point.y, -kInfinity),
                             std::nextafter(point.x, kInfinity), std::nextafter(point.y, kInfinity));
      geometry.m_triangleIndex->ForEachInRect(bounds, [&](size_t index) { attach(geometry.m_triangles[index]); });
    }
    else
      for (auto const & triangle : geometry.m_triangles)
        attach(triangle);
  }

  report(0.25);
  std::set<Point> shore;
  std::map<Point, std::vector<Point>> previous, next;
  for (auto const & [key, edge] : surfaceEdges)
    if (edge.m_count == 1)
    {
      shore.insert(edge.m_from);
      shore.insert(edge.m_to);
      previous[edge.m_to].push_back(edge.m_from);
      next[edge.m_from].push_back(edge.m_to);
    }
  // Water lies to the left of oriented boundary edges. A shortest detour turns
  // only at a right turn (reflex water angle), including convex island corners.
  std::erase_if(shore, [&](Point const & point)
  {
    auto const & before = previous[point];
    auto const & after = next[point];
    return before.size() == 1 && after.size() == 1 && m2::robust::OrientedS(before[0], point, after[0]) >= 0;
  });
  // Include closely spaced boundary goals so A* can bound the cost of leaving
  // the extraction window, including exits between stored triangle vertices.
  double constexpr kFrontierSpacing = 50.0;
  if (geometry.m_bounds)
  {
    auto const & bounds = *geometry.m_bounds;
    auto const onSameSide = [&](Point const & a, Point const & b)
    {
      for (size_t side = 0; side < 4; ++side)
        if (OnSide(bounds, a, side) && OnSide(bounds, b, side))
          return true;
      return false;
    };
    auto const addFrontier = [&](size_t a, size_t b, bool surface)
    {
      auto const from = graph.m_points[a], to = graph.m_points[b];
      size_t const steps = std::max<size_t>(1, std::ceil(BoundaryDistanceBound(from, to) / kFrontierSpacing));
      if (steps - 1 > kMaxGraphVertices - graph.m_points.size())
        return false;
      auto previous = a;
      for (size_t step = 1; step < steps; ++step)
      {
        double const parameter = static_cast<double>(step) / steps;
        Point point = from + (to - from) * parameter;
        if (surface && !MakePortal(geometry, from, to, parameter, (step + 0.5) / steps, point))
          return false;
        auto const node = graph.Add(point);
        graph.Connect(previous, node);
        graph.m_frontierEdges.emplace_back(previous, node);
        previous = node;
      }
      graph.Connect(previous, b);
      graph.m_frontierEdges.emplace_back(previous, b);
      return true;
    };
    std::vector<std::pair<size_t, size_t>> lineFrontiers = graph.m_unknownEdges;
    for (size_t a = 0; a < graph.m_points.size(); ++a)
      for (auto const & edge : graph.m_edges[a])
        if (a < edge.m_target && onSameSide(graph.m_points[a], graph.m_points[edge.m_target]))
          lineFrontiers.emplace_back(a, edge.m_target);
    for (auto const & [key, edge] : surfaceEdges)
      if (edge.m_count == 1 && onSameSide(edge.m_from, edge.m_to) &&
          !addFrontier(surfaceVertices.at(edge.m_from), surfaceVertices.at(edge.m_to), true))
        return ResultCode::Unavailable;
    for (auto const & [a, b] : lineFrontiers)
    {
      if (cancelled())
        return ResultCode::Cancelled;
      size_t visits = 0;
      if (!onSameSide(graph.m_points[a], graph.m_points[b]))
        continue;
      bool const covered = CoversSegment(geometry, graph.m_points[a], graph.m_points[b], cancelled, visits);
      if (visits > kMaxVisibilityTriangleVisits || (!covered && !addFrontier(a, b, false)))
        return ResultCode::Unavailable;
    }
  }
  std::vector<bool> surface(graph.m_points.size());
  std::vector<size_t> visibility;
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    surface[i] = surfaceVertices.contains(graph.m_points[i]) || IsSurfacePoint(geometry, graph.m_points[i]);
    // Interior mesh vertices need only linear triangle edges. Shortest surface
    // detours turn at the shore, checkpoints or river/surface intersections.
    // Submerged river bends remain in the sparse graph but need no visibility edges.
    bool boundary = false;
    if (geometry.m_bounds)
      for (size_t side = 0; side < 4; ++side)
        boundary = boundary || OnSide(*geometry.m_bounds, graph.m_points[i], side);
    if (surface[i] &&
        (boundary || surfacePortals.contains(i) || graph.m_lineEndpoints[i] || shore.contains(graph.m_points[i])))
      visibility.push_back(i);
  }
  report(0.3);
  auto const component = Dijkstra(graph, 0, cancelled);
  double const frontierMargin = geometry.m_bounds ? 4 * kFrontierSpacing + 0.1 : 0;
  report(0.35);
  auto const forward = SearchWater(graph, 1, visibility, component, cancelled,
                                   [&](double value) { report(0.35 + 0.35 * value); }, frontierMargin);
  if (cancelled())
    return ResultCode::Cancelled;
  if (forward.m_budgetExceeded && forward.m_distances[1] == kInfinity)
    return ResultCode::Unavailable;
  if (forward.m_distances[1] != kInfinity)
  {
    auto nodes = Trace(forward, 1);
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
      Append(path, graph.m_points[*it], false);
    // A confirmed path remains useful even when missing coverage or an exhausted
    // search budget prevents proving that no shorter alternative exists.
    path.m_shortestConfirmed =
        !forward.m_budgetExceeded && !UnknownCouldShorten(geometry, from.m_point, to.m_point, path.m_waterDistance) &&
        !(geometry.m_bounds && ReachesFrontier(geometry, graph, 0, false) &&
          OutsideCouldShorten(geometry, graph, forward, 0, to.m_point, path.m_waterDistance, cancelled));
    if (cancelled())
    {
      path = {};
      return ResultCode::Cancelled;
    }
    report(1.0);
    return ResultCode::Success;
  }

  // Search the local water graph before considering a walking transfer.
  report(0.7);
  if (allowUnconfirmedTransfer)
    return FindWalkingPath(graph, shore, cancelled, path, progress, walkAllowed);
  auto const backwardComponent = Dijkstra(graph, 1, cancelled);
  auto const backward = SearchWater(graph, 0, visibility, backwardComponent, cancelled,
                                    [&](double value) { report(0.7 + 0.15 * value); });
  if (cancelled())
    return ResultCode::Cancelled;
  if (backward.m_budgetExceeded)
    return ResultCode::Unavailable;
  bool const openFrontier = ReachesFrontier(geometry, graph, 0) || ReachesFrontier(geometry, graph, 1);
  if (openFrontier && geometry.m_portageAreas.empty())
    return ResultCode::Unavailable;
  std::vector<size_t> transfers;
  for (size_t i = 0; i < graph.m_points.size(); ++i)
    if ((graph.m_lineEndpoints[i] || (geometry.m_hasSourceTopology && surface[i])) &&
        (forward.m_distances[i] != kInfinity || backward.m_distances[i] != kInfinity))
      transfers.push_back(i);
  if (transfers.size() > kMaxTransferVertices)
    return ResultCode::Unavailable;
  double best = kInfinity;
  size_t gapFrom = 0, gapTo = 1;
  Interval gap;
  size_t transferCount = 0;
  for (auto const a : transfers)
  {
    report(0.85 + 0.14 * transferCount++ / transfers.size());
    if (cancelled())
      return ResultCode::Cancelled;
    if (forward.m_distances[a] == kInfinity)
      continue;
    for (auto const b : transfers)
    {
      if (backward.m_distances[b] == kInfinity)
        continue;
      // A mapped underground break permits a local walking bypass even when
      // the bounded water search cannot exclude a distant water alternative.
      auto const knownPortage =
          std::any_of(geometry.m_portageAreas.begin(), geometry.m_portageAreas.end(), [&](auto const & rect)
      {
        return Intersects(rect, graph.m_points[a], graph.m_points[b]) &&
               DistanceToUnknown(rect, graph.m_points[a]) <= 50.0 && DistanceToUnknown(rect, graph.m_points[b]) <= 50.0;
      });
      if (openFrontier && !knownPortage)
        continue;
      // A walking bypass keeps the actual river endpoints, including endpoints
      // inside a water surface; never trim it to arbitrary shoreline points.
      auto const coverage =
          knownPortage ? std::vector<Interval>{} : Coverage(geometry, graph.m_points[a], graph.m_points[b]);
      double begin = 0.0, end = 1.0;
      size_t index = 0;
      if (!coverage.empty() && coverage.front().first == 0.0)
      {
        begin = coverage.front().second;
        ++index;
      }
      if (index < coverage.size() && coverage.back().second == 1.0)
        end = coverage.back().first;
      else if (index != coverage.size())
        continue;
      // Reject a ray which visits a third water system or needs multiple land gaps.
      if (index + (end < 1.0 ? 1 : 0) != coverage.size() || begin >= end)
        continue;
      if (graph.m_points[a] == graph.m_points[b])
        continue;
      // Display geometry permits only stored line endpoints, never shoreline cuts.
      if (!geometry.m_hasSourceTopology && (begin != 0.0 || end != 1.0))
        continue;
      // River transfers use source line endpoints, never checkpoints or extraction cuts.
      // Surface transfers use the actual shoreline intersections of the candidate ray.
      if ((begin == 0.0 && !graph.m_lineEndpoints[a] && !IsSurfacePoint(geometry, graph.m_points[a])) ||
          (end == 1.0 && !graph.m_lineEndpoints[b] && !IsSurfacePoint(geometry, graph.m_points[b])))
        continue;
      auto const delta = graph.m_points[b] - graph.m_points[a];
      auto const exit = begin == 0.0 ? graph.m_points[a] : graph.m_points[a] + delta * begin;
      auto const entry = end == 1.0 ? graph.m_points[b] : graph.m_points[a] + delta * end;
      if (TouchesUnknown(geometry, graph.m_points[a], graph.m_points[b]))
        continue;
      auto const distance =
          forward.m_distances[a] + backward.m_distances[b] + mercator::DistanceOnEarth(graph.m_points[a], exit) +
          mercator::DistanceOnEarth(exit, entry) + mercator::DistanceOnEarth(entry, graph.m_points[b]);
      if (distance < best)
      {
        best = distance;
        gapFrom = a;
        gapTo = b;
        gap = {begin, end};
      }
    }
  }
  if (best == kInfinity || UnknownCouldShorten(geometry, from.m_point, to.m_point, best))
    return ResultCode::Unavailable;

  path.m_shortestConfirmed = false;
  auto nodes = Trace(forward, gapFrom);
  for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
    Append(path, graph.m_points[*it], false);
  auto const a = graph.m_points[gapFrom];
  auto const delta = graph.m_points[gapTo] - a;
  Append(path, gap.first == 0.0 ? a : a + delta * gap.first, false);
  Append(path, gap.second == 1.0 ? graph.m_points[gapTo] : a + delta * gap.second, true);
  Append(path, graph.m_points[gapTo], false);
  for (auto node : Trace(backward, gapTo))
    Append(path, graph.m_points[node], false);
  report(1.0);
  return ResultCode::Success;
}
}  // namespace routing::water
