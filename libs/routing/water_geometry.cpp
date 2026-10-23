#include "routing/water_geometry.hpp"

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
#include <utility>

namespace routing::water
{
namespace
{
using Point = m2::PointD;
using Interval = std::pair<double, double>;
double constexpr kInfinity = std::numeric_limits<double>::max();
size_t constexpr kMaxGraphVertices = 65536;
size_t constexpr kMaxTransferVertices = 2048;
size_t constexpr kMaxTriangles = 16384;
size_t constexpr kMaxVisibilityChecks = 2000000;
size_t constexpr kMaxVisibilityTriangleVisits = 8000000;

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

  size_t Add(Point const & p, bool lineEndpoint = false)
  {
    m_points.push_back(p);
    m_edges.emplace_back();
    m_lineEndpoints.push_back(lineEndpoint);
    m_unknownFrontier.push_back(TouchesUnknown(m_geometry, p, p));
    return m_points.size() - 1;
  }

  void Connect(size_t a, size_t b)
  {
    if (TouchesUnknown(m_geometry, m_points[a], m_points[b]))
    {
      m_unknownFrontier[a] = m_unknownFrontier[b] = true;
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
                   CancelledFn const & isCancelled)
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
  auto const source = target == 1 ? 0 : 1;
  for (auto const node : visibility)
    if (result.m_distances[target] == kInfinity ||
        ms::DistanceOnEarth(coordinates[source], coordinates[node]) + estimates[node] < result.m_distances[target])
      candidates.push_back(node);
  using Item = std::pair<double, size_t>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
  for (size_t node = 0; node < graph.m_points.size(); ++node)
    if (result.m_distances[node] != kInfinity)
      queue.emplace(result.m_distances[node] + estimates[node], node);
  size_t checks = 0;
  size_t triangleVisits = 0;
  while (!queue.empty() && !cancelled())
  {
    auto const [score, node] = queue.top();
    queue.pop();
    if (settled[node] || score > result.m_distances[node] + estimates[node])
      continue;
    settled[node] = true;
    if (node == target)
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
      // With no unknown areas, the conforming sparse mesh already certifies
      // components. A visibility edge cannot cross between disconnected waters.
      if (graph.m_geometry.m_unknownAreas.empty() && component.m_distances[other] == kInfinity)
        continue;
      auto const distance = ms::DistanceOnEarth(coordinates[node], coordinates[other]);
      if (result.m_distances[node] + distance >= result.m_distances[other])
        continue;
      if (result.m_distances[target] != kInfinity &&
          result.m_distances[node] + distance + estimates[other] >= result.m_distances[target])
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

Selection Select(Geometry const & geometry, Point const & point)
{
  if (IsSurfacePoint(geometry, point))
    return {point, std::numeric_limits<size_t>::max(), 0, 0.0};
  Selection result;
  auto const consider = [&](Point const & a, Point const & b, size_t line, size_t segment)
  {
    auto const projection = Project(point, a, b);
    auto const distance = mercator::DistanceOnEarth(point, projection);
    if (distance < result.m_distance)
      result = {projection, line, segment, distance};
  };
  for (size_t i = 0; i < geometry.m_lines.size(); ++i)
  {
    auto const & line = geometry.m_lines[i];
    for (size_t j = 1; j < line.size(); ++j)
      consider(line[j - 1].m_point, line[j].m_point, i, j);
  }
  for (auto const & triangle : geometry.m_triangles)
    for (size_t i = 0; i < 3; ++i)
      consider(triangle[i], triangle[(i + 1) % 3], std::numeric_limits<size_t>::max(), 0);
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

bool OutsideCouldShorten(Geometry const & geometry, Point const & from, Point const & to, double distance,
                         CancelledFn const & cancelled)
{
  auto const & bounds = *geometry.m_bounds;
  if (ExitDistance(bounds, from) + ExitDistance(bounds, to) >= distance + 0.01)
    return false;
  // Any excursion touches a boundary point p. Triangle inequality bounds its
  // length below by d(from,p)+d(p,to), even when it exits and reenters elsewhere.
  auto const check = [&](auto && self, Point const & a, Point const & b, size_t depth) -> bool
  {
    if (cancelled())
      return true;
    auto const middle = (a + b) * 0.5;
    auto const sum = mercator::DistanceOnEarth(from, middle) + mercator::DistanceOnEarth(middle, to);
    double radius;
    if (a.x == b.x)
      radius = ms::kEarthRadiusMeters *
               math::DegToRad(std::max(std::abs(mercator::YToLat(a.y) - mercator::YToLat(middle.y)),
                                       std::abs(mercator::YToLat(b.y) - mercator::YToLat(middle.y))));
    else
      radius = ms::kEarthRadiusMeters * math::DegToRad(std::abs(b.x - a.x) * 0.5) *
               std::cos(math::DegToRad(mercator::YToLat(a.y)));
    // The sum of distances is 2-Lipschitz with respect to travel along the edge.
    if (sum - 2 * radius >= distance + 0.01)
      return false;
    if (sum < distance || depth == 12)
      return true;
    return self(self, a, middle, depth + 1) || self(self, middle, b, depth + 1);
  };
  auto const onSide = [&](Point const & p, size_t side)
  {
    switch (side)
    {
    case 0: return p.x <= bounds.minX() + 1e-12;
    case 1: return p.x >= bounds.maxX() - 1e-12;
    case 2: return p.y <= bounds.minY() + 1e-12;
    default: return p.y >= bounds.maxY() - 1e-12;
    }
  };
  // An unknown continuation must leave through mapped water, not through dry
  // parts of the rectangle. Provider geometry is clipped to these bounds.
  for (auto const & triangle : geometry.m_triangles)
    for (size_t i = 0; i < 3; ++i)
      for (size_t side = 0; side < 4; ++side)
      {
        auto const & a = triangle[i];
        auto const & b = triangle[(i + 1) % 3];
        if (onSide(a, side) && check(check, a, onSide(b, side) ? b : a, 0))
          return true;
      }
  for (auto const & line : geometry.m_lines)
    for (auto const & vertex : line)
      for (size_t side = 0; side < 4; ++side)
        if (onSide(vertex.m_point, side) && check(check, vertex.m_point, vertex.m_point, 0))
          return true;
  return false;
}

bool ReachesFrontier(Geometry const & geometry, Graph const & graph, Search const & search, bool includeUnknown = true)
{
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    if (search.m_distances[i] == kInfinity)
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
}  // namespace

ResultCode FindPath(Geometry const & geometry, Point const & start, Point const & finish, bool projectStart,
                    CancelledFn const & cancelled, Path & path)
{
  path = {};
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
    return ResultCode::Success;
  }
  size_t triangleVisits = 0;
  if (CoversSegment(geometry, from.m_point, to.m_point, cancelled, triangleVisits) &&
      !TouchesUnknown(geometry, from.m_point, to.m_point))
  {
    Append(path, from.m_point, false);
    Append(path, to.m_point, false);
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
  for (size_t i = 0; i < geometry.m_lines.size(); ++i)
  {
    if (cancelled())
      return ResultCode::Cancelled;
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
        split.push_back(graph.Add(entrance));
        split.push_back(graph.Add(exit));
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
  for (auto const & triangle : geometry.m_triangles)
  {
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

  // Attach checkpoints and river portals to the sparse water mesh before the
  // lazy visibility search. This also preserves component/frontier reachability.
  for (size_t i = 0; i < lineVertices; ++i)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    auto const & point = graph.m_points[i];
    auto const attach = [&](std::array<Point, 3> const & triangle)
    {
      Interval interval;
      if (!Clip(point, point, triangle, interval))
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
  std::vector<bool> surface(graph.m_points.size());
  std::vector<size_t> visibility;
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    surface[i] = surfaceVertices.contains(graph.m_points[i]) || IsSurfacePoint(geometry, graph.m_points[i]);
    // Interior mesh vertices need only linear triangle edges. Shortest surface
    // detours turn at the shore, checkpoints or river/surface intersections.
    if (surface[i] && (i < lineVertices || shore.contains(graph.m_points[i])))
      visibility.push_back(i);
  }
  auto const component = Dijkstra(graph, 0, cancelled);
  auto const forward = SearchWater(graph, 1, visibility, component, cancelled);
  if (cancelled())
    return ResultCode::Cancelled;
  if (forward.m_budgetExceeded)
    return ResultCode::Unavailable;
  if (forward.m_distances[1] != kInfinity)
  {
    auto nodes = Trace(forward, 1);
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
      Append(path, graph.m_points[*it], false);
    // Any path leaving complete coverage must first travel at least this far.
    // If that bound cannot exclude a shorter outside path, report unavailable.
    if (UnknownCouldShorten(geometry, from.m_point, to.m_point, path.m_waterDistance) ||
        (geometry.m_bounds && ReachesFrontier(geometry, graph, component, false) &&
         OutsideCouldShorten(geometry, from.m_point, to.m_point, path.m_waterDistance, cancelled)))
    {
      path = {};
      return cancelled() ? ResultCode::Cancelled : ResultCode::Unavailable;
    }
    return ResultCode::Success;
  }

  // A land gap is considered only after exhausting the complete water graph.
  auto const backwardComponent = Dijkstra(graph, 1, cancelled);
  auto const backward = SearchWater(graph, 0, visibility, backwardComponent, cancelled);
  if (cancelled())
    return ResultCode::Cancelled;
  if (backward.m_budgetExceeded)
    return ResultCode::Unavailable;
  if (ReachesFrontier(geometry, graph, forward) || ReachesFrontier(geometry, graph, backward))
    return ResultCode::Unavailable;
  std::vector<size_t> transfers;
  for (size_t i = 0; i < graph.m_points.size(); ++i)
    if (graph.m_lineEndpoints[i] || (geometry.m_hasSourceTopology && surface[i]))
      transfers.push_back(i);
  if (transfers.size() > kMaxTransferVertices)
    return ResultCode::Unavailable;
  double best = kInfinity;
  size_t gapFrom = 0, gapTo = 1;
  Interval gap;
  for (auto const a : transfers)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    if (forward.m_distances[a] == kInfinity)
      continue;
    for (auto const b : transfers)
    {
      if (backward.m_distances[b] == kInfinity)
        continue;
      auto const coverage = Coverage(geometry, graph.m_points[a], graph.m_points[b]);
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
  return ResultCode::Success;
}
}  // namespace routing::water
