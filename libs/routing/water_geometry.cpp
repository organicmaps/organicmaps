#include "routing/water_geometry.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <utility>

namespace routing::water
{
namespace
{
using Point = m2::PointD;
using Interval = std::pair<double, double>;
double constexpr kInfinity = std::numeric_limits<double>::max();
size_t constexpr kMaxVertices = 1024;

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
    geometry.m_triangleIndex->ForEachInRect(m2::RectD(a, b), [&](size_t i) { visit(geometry.m_triangles[i]); });
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

bool MakePortal(Geometry const & geometry, Point const & a, Point const & b, double t, double interior, Point & point)
{
  // A certified intersection can round outside its triangle. Move only in parameter-space ULPs
  // toward the already certified interior, never by a geographic proximity threshold.
  for (size_t attempt = 0; attempt < 16; ++attempt)
  {
    point = t == 0.0 ? a : (t == 1.0 ? b : a + (b - a) * t);
    if (IsSurfacePoint(geometry, point))
      return true;
    t = std::nextafter(t, interior);
  }
  return false;
}

struct Edge
{
  size_t m_target;
  double m_distance;
};

struct Graph
{
  std::vector<Point> m_points;
  std::vector<std::vector<Edge>> m_edges;
  std::vector<bool> m_lineEndpoints;

  size_t Add(Point const & p, bool lineEndpoint = false)
  {
    m_points.push_back(p);
    m_edges.emplace_back();
    m_lineEndpoints.push_back(lineEndpoint);
    return m_points.size() - 1;
  }

  void Connect(size_t a, size_t b)
  {
    auto const distance = mercator::DistanceOnEarth(m_points[a], m_points[b]);
    m_edges[a].push_back({b, distance});
    m_edges[b].push_back({a, distance});
  }
};

struct Search
{
  std::vector<double> m_distances;
  std::vector<size_t> m_previous;
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

bool ReachesFrontier(Geometry const & geometry, Graph const & graph, Search const & search)
{
  if (!geometry.m_bounds)
    return false;
  auto const & bounds = *geometry.m_bounds;
  for (size_t i = 0; i < graph.m_points.size(); ++i)
  {
    if (search.m_distances[i] == kInfinity)
      continue;
    auto const & p = graph.m_points[i];
    if (p.x <= bounds.minX() + 1e-12 || p.x >= bounds.maxX() - 1e-12 || p.y <= bounds.minY() + 1e-12 ||
        p.y >= bounds.maxY() - 1e-12)
      return true;
  }
  return false;
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
  if (geometry.m_triangles.size() > 4 * kMaxVertices || geometry.m_lines.size() > kMaxVertices)
    return ResultCode::Unavailable;
  for (auto const & line : geometry.m_lines)
  {
    if (line.size() > kMaxVertices - std::min(count, kMaxVertices))
      return ResultCode::Unavailable;
    count += line.size();
  }
  if (count > kMaxVertices)
    return ResultCode::Unavailable;

  auto const from = Select(geometry, start);
  auto const to = Select(geometry, finish);
  // Explicit selections must actually be on the supplied geometry. Only the implicit GPS start projects.
  if (from.m_distance == kInfinity || to.m_distance == kInfinity)
    return ResultCode::Unavailable;
  // Permit only the sub-decimetre quantization error of a selected MWM line.
  if ((!projectStart && from.m_distance > 0.1) || to.m_distance > 0.1)
    return ResultCode::OutsideWater;
  if (geometry.m_bounds &&
      (ExitDistance(*geometry.m_bounds, from.m_point) == 0 || ExitDistance(*geometry.m_bounds, to.m_point) == 0 ||
       (projectStart && from.m_distance >= ExitDistance(*geometry.m_bounds, start))))
    return ResultCode::Unavailable;

  Graph graph;
  graph.Add(from.m_point);
  graph.Add(to.m_point);
  std::map<uint64_t, size_t> sourceNodes;
  for (size_t i = 0; i < geometry.m_lines.size(); ++i)
  {
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
        Point entrance, exit;
        if (!MakePortal(geometry, origin, line[j].m_point, interval.first, interval.second, entrance) ||
            !MakePortal(geometry, origin, line[j].m_point, interval.second, interval.first, exit))
          return ResultCode::Unavailable;
        split.push_back(graph.Add(entrance));
        split.push_back(graph.Add(exit));
      }
      if (graph.m_points.size() > kMaxVertices)
        return ResultCode::Unavailable;
      std::sort(split.begin(), split.end(), [&](size_t a, size_t b)
      { return (graph.m_points[a] - origin).SquaredLength() < (graph.m_points[b] - origin).SquaredLength(); });
      for (size_t k = 1; k < split.size(); ++k)
        graph.Connect(split[k - 1], split[k]);
    }
  }
  std::map<Point, size_t> surfaceVertices;
  for (auto const & triangle : geometry.m_triangles)
    for (auto const & point : triangle)
      if (surfaceVertices.emplace(point, 0).second)
      {
        if (graph.m_points.size() >= kMaxVertices)
          return ResultCode::Unavailable;
        graph.Add(point);
      }

  for (size_t a = 0; a < graph.m_points.size(); ++a)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    for (size_t b = a + 1; b < graph.m_points.size(); ++b)
    {
      auto const coverage = Coverage(geometry, graph.m_points[a], graph.m_points[b]);
      if (coverage.size() == 1 && coverage[0].first == 0.0 && coverage[0].second == 1.0)
        graph.Connect(a, b);
    }
  }

  auto const forward = Dijkstra(graph, 0, cancelled);
  if (cancelled())
    return ResultCode::Cancelled;
  if (forward.m_distances[1] != kInfinity)
  {
    auto nodes = Trace(forward, 1);
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
      Append(path, graph.m_points[*it], false);
    // Any path leaving complete coverage must first travel at least this far.
    // If that bound cannot exclude a shorter outside path, report unavailable.
    if (ReachesFrontier(geometry, graph, forward) &&
        path.m_waterDistance >= ExitDistance(*geometry.m_bounds, from.m_point))
    {
      path = {};
      return ResultCode::Unavailable;
    }
    return ResultCode::Success;
  }

  // A land gap is considered only after exhausting the complete water graph.
  auto const backward = Dijkstra(graph, 1, cancelled);
  if (ReachesFrontier(geometry, graph, forward) || ReachesFrontier(geometry, graph, backward))
    return ResultCode::Unavailable;
  double best = kInfinity;
  size_t gapFrom = 0, gapTo = 1;
  Interval gap;
  for (size_t a = 0; a < graph.m_points.size(); ++a)
  {
    if (cancelled())
      return ResultCode::Cancelled;
    if (forward.m_distances[a] == kInfinity)
      continue;
    for (size_t b = 0; b < graph.m_points.size(); ++b)
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
      // River transfers use source line endpoints, never checkpoints or extraction cuts.
      // Surface transfers use the actual shoreline intersections of the candidate ray.
      if ((begin == 0.0 && !graph.m_lineEndpoints[a] && !IsSurfacePoint(geometry, graph.m_points[a])) ||
          (end == 1.0 && !graph.m_lineEndpoints[b] && !IsSurfacePoint(geometry, graph.m_points[b])))
        continue;
      auto const delta = graph.m_points[b] - graph.m_points[a];
      auto const exit = graph.m_points[a] + delta * begin;
      auto const entry = graph.m_points[a] + delta * end;
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
  if (best == kInfinity)
    return ResultCode::Unavailable;

  auto nodes = Trace(forward, gapFrom);
  for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
    Append(path, graph.m_points[*it], false);
  auto const a = graph.m_points[gapFrom];
  auto const delta = graph.m_points[gapTo] - a;
  Append(path, a + delta * gap.first, false);
  Append(path, a + delta * gap.second, true);
  Append(path, graph.m_points[gapTo], false);
  for (auto node : Trace(backward, gapTo))
    Append(path, graph.m_points[node], false);
  return ResultCode::Success;
}
}  // namespace routing::water
