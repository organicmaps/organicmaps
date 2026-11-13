#include "routing/water_geometry_provider.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/polygon.hpp"
#include "geometry/robust_orientation.hpp"

#include "3party/libtess2/Include/tesselator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <set>
#include <tuple>

namespace routing
{
namespace
{
using Point = m2::PointD;
size_t constexpr kMaxInputPoints = 16384;
size_t constexpr kMaxSurfacePoints = 4096;
using water::kMaxTriangles;
size_t constexpr kMaxStandardMaps = 16;

// Clip a convex contour against a rectangle. Intersections use a canonical edge
// direction so neighbouring triangles reconstruct identical boundary points.
std::vector<Point> Clip(std::vector<Point> polygon, m2::RectD const & bounds)
{
  for (size_t side = 0; side < 4 && !polygon.empty(); ++side)
  {
    auto const axis = side / 2;
    double const value =
        axis == 0 ? (side == 0 ? bounds.minX() : bounds.maxX()) : (side == 2 ? bounds.minY() : bounds.maxY());
    auto const coord = [axis](Point const & p) { return axis == 0 ? p.x : p.y; };
    auto const inside = [&](Point const & p) { return side % 2 == 0 ? coord(p) >= value : coord(p) <= value; };
    std::vector<Point> output;
    for (size_t i = 0; i < polygon.size(); ++i)
    {
      auto a = polygon[i];
      auto b = polygon[(i + 1) % polygon.size()];
      if (inside(a))
        output.push_back(a);
      if (inside(a) == inside(b))
        continue;
      if (b < a)
        std::swap(a, b);
      auto p = a + (b - a) * ((value - coord(a)) / (coord(b) - coord(a)));
      (axis == 0 ? p.x : p.y) = value;
      output.push_back(p);
    }
    polygon = std::move(output);
  }
  return polygon;
}

bool AddSurface(std::vector<::water::Record> const & records, m2::RectD const & bounds,
                water::CancelledFn const & cancelled, water::Geometry & result,
                size_t maxSurfacePoints = kMaxSurfacePoints)
{
  std::unique_ptr<TESStesselator, decltype(&tessDeleteTess)> tess(tessNewTess(nullptr), tessDeleteTess);
  if (!tess)
    return false;
  bool any = false;
  size_t points = 0;
  for (auto const & record : records)
  {
    if (cancelled())
      return false;
    if (record.m_kind != ::water::Kind::Area)
      continue;
    auto rings = record.m_rings;
    bool valid = true;
    for (auto & ring : rings)
    {
      if (ring.size() > maxSurfacePoints - points)
        return false;
      points += ring.size();
      if (ring.size() > 1 && ring.front() == ring.back())
        ring.pop_back();
      bool hasArea = false;
      for (size_t i = 2; i < ring.size(); ++i)
        hasArea = hasArea || m2::robust::OrientedS(ring[0], ring[i - 1], ring[i]) != 0.0;
      if (ring.size() < 3 || !hasArea || m2::robust::CheckPolygonSelfIntersections(ring.begin(), ring.end()))
        valid = false;
    }
    if (!valid)
    {
      result.m_unknownAreas.push_back(record.GetBounds());
      continue;
    }
    for (size_t i = 0; i < rings.size(); ++i)
    {
      auto & ring = rings[i];
      // NONZERO winding forms the polygon union, preserving holes and adding
      // intersections at overlaps before visibility vertices are selected.
      if (IsPolygonCCW(ring.begin(), ring.end()) != (i == 0))
        std::reverse(ring.begin(), ring.end());
      tessAddContour(tess.get(), 2, ring.data(), sizeof(Point), static_cast<int>(ring.size()));
      any = true;
    }
  }
  if (!any)
    return true;
  if (!tessTesselate(tess.get(), TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 2, nullptr) || cancelled())
    return false;
  auto const * vertices = tessGetVertices(tess.get());
  auto const * triangles = tessGetElements(tess.get());
  auto const count = tessGetElementCount(tess.get());
  for (int i = 0; i < count; ++i)
  {
    if (cancelled())
      return false;
    std::vector<Point> triangle;
    for (int j = 0; j < 3; ++j)
    {
      auto const index = triangles[3 * i + j];
      if (index == TESS_UNDEF)
        return false;
      triangle.emplace_back(vertices[2 * index], vertices[2 * index + 1]);
    }
    auto const clipped = Clip(std::move(triangle), bounds);
    for (size_t j = 2; j < clipped.size(); ++j)
    {
      if (m2::robust::OrientedS(clipped[0], clipped[j - 1], clipped[j]) == 0)
        continue;
      if (result.m_triangles.size() >= kMaxTriangles)
        return false;
      result.m_triangles.push_back({clipped[0], clipped[j - 1], clipped[j]});
    }
  }
  return true;
}

struct FeatureSurface
{
  std::vector<::water::Record> m_triangles;
};

bool AddFeatureSurfaces(std::vector<FeatureSurface> & surfaces, m2::RectD const & bounds,
                        water::CancelledFn const & cancelled, water::Geometry & result)
{
  std::vector<size_t> parents(surfaces.size());
  std::iota(parents.begin(), parents.end(), 0);
  auto const root = [&](size_t node)
  {
    while (parents[node] != node)
    {
      parents[node] = parents[parents[node]];
      node = parents[node];
    }
    return node;
  };
  // Feature bounds can enclose many unrelated lakes along a winding river.
  // Use triangle bounds conservatively: real intersections and contacts still join.
  {
    std::vector<std::array<Point, 3>> triangles;
    std::vector<size_t> owners;
    for (size_t i = 0; i < surfaces.size(); ++i)
      for (auto const & record : surfaces[i].m_triangles)
      {
        auto const & ring = record.m_rings.front();
        triangles.push_back({ring[0], ring[1], ring[2]});
        owners.push_back(i);
      }
    water::TriangleIndex index(triangles);
    for (size_t i = 0; i < triangles.size(); ++i)
    {
      if (cancelled())
        return false;
      m2::RectD triangleBounds;
      for (auto const & point : triangles[i])
        triangleBounds.Add(point);
      index.ForEachInRect(triangleBounds, [&](size_t j)
      {
        if (j >= i || owners[i] == owners[j])
          return;
        auto const a = root(owners[i]), b = root(owners[j]);
        parents[std::max(a, b)] = std::min(a, b);
      });
    }
  }
  for (size_t i = 0; i < surfaces.size(); ++i)
  {
    auto const group = root(i);
    if (group == i)
      continue;
    auto & triangles = surfaces[group].m_triangles;
    for (auto & record : surfaces[i].m_triangles)
      triangles.push_back(std::move(record));
    surfaces[i].m_triangles.clear();
  }
  // Disjoint triangle-bounds groups cannot overlap or touch. Union each group
  // independently, retaining every triangle, island and exact surface contact.
  for (auto const & surface : surfaces)
    if (!surface.m_triangles.empty() && !AddSurface(surface.m_triangles, bounds, cancelled, result, 3 * kMaxTriangles))
      return false;
  return true;
}

// Keep uncovered corners explicit instead of rejecting a whole query rectangle.
// Fully unknown sibling cells collapse back to their parent, bounding the number
// of rectangles tested by the water graph.
template <typename Covered>
void AddCoverageGaps(m2::RectD const & bounds, std::vector<Point> const & checkpoints, Covered const & covered,
                     water::CancelledFn const & cancelled, std::vector<m2::RectD> & unknown)
{
  auto const visit = [&](auto && self, m2::RectD const & cell, size_t depth) -> bool
  {
    if (cancelled() || covered(cell))
      return false;
    // Resolve the checkpoint cells more finely so a nearby missing corner
    // cannot hide a point with certified coverage.
    bool const checkpointCell = std::any_of(checkpoints.begin(), checkpoints.end(),
                                            [&](auto const & point) { return cell.IsPointInside(point); });
    if (depth == 16 || (depth >= 6 && !checkpointCell))
    {
      unknown.push_back(cell);
      return true;
    }
    auto const begin = unknown.size();
    auto const middle = cell.Center();
    bool allUnknown = true;
    for (auto const & child : {m2::RectD(cell.LeftBottom(), middle), m2::RectD(middle, cell.RightTop()),
                               m2::RectD(cell.minX(), middle.y, middle.x, cell.maxY()),
                               m2::RectD(middle.x, cell.minY(), cell.maxX(), middle.y)})
      allUnknown = self(self, child, depth + 1) && allUnknown;
    if (allUnknown)
    {
      unknown.resize(begin);
      unknown.push_back(cell);
    }
    return allUnknown;
  };
  visit(visit, bounds, 0);
}
void AddLine(::water::Record const & record, m2::RectD const & bounds, water::Geometry & result)
{
  auto const & points = record.m_rings.front();
  std::vector<water::Vertex> fragment;
  auto const flush = [&]
  {
    if (fragment.size() >= 2)
      result.m_lines.push_back(std::move(fragment));
    fragment.clear();
  };
  for (size_t i = 1; i < points.size(); ++i)
  {
    auto const a = points[i - 1];
    auto const b = points[i];
    double begin = 0, end = 1;
    auto const delta = b - a;
    bool valid = true;
    for (auto const [p, q] : std::array<std::pair<double, double>, 4>{std::pair{-delta.x, a.x - bounds.minX()},
                                                                      {delta.x, bounds.maxX() - a.x},
                                                                      {-delta.y, a.y - bounds.minY()},
                                                                      {delta.y, bounds.maxY() - a.y}})
    {
      if (p == 0)
      {
        valid = valid && q >= 0;
        continue;
      }
      auto const t = q / p;
      if (p < 0)
        begin = std::max(begin, t);
      else
        end = std::min(end, t);
    }
    if (!valid || begin >= end)
    {
      flush();
      continue;
    }
    auto const vertex = [&](double t, bool first)
    {
      auto p = t == 0 ? a : (t == 1 ? b : a + delta * t);
      p.x = std::clamp(p.x, bounds.minX(), bounds.maxX());
      p.y = std::clamp(p.y, bounds.minY(), bounds.maxY());
      auto const original = first ? t == 0 : t == 1;
      return water::Vertex{p, original ? record.m_nodes[first ? i - 1 : i] : 0,
                           original && (first ? i == 1 : i + 1 == points.size())};
    };
    auto const from = vertex(begin, true);
    auto const to = vertex(end, false);
    if (fragment.empty() || fragment.back().m_point != from.m_point)
    {
      flush();
      fragment.push_back(from);
    }
    fragment.push_back(to);
    if (end != 1)
      flush();
  }
  flush();
}

void IndexTriangles(water::Geometry & geometry)
{
  geometry.m_triangleIndex = std::make_shared<water::TriangleIndex>(geometry.m_triangles);
}
}  // namespace

water::Geometry WaterGeometryProvider::Extract(::water::Reader const & reader, m2::RectD const & bounds,
                                               water::CancelledFn const & cancelled)
{
  water::Geometry result;
  std::vector<::water::Record> records;
  if (!reader.Read(bounds, kMaxInputPoints, cancelled, records))
    return result;
  for (auto const & record : records)
  {
    if (cancelled())
      return {};
    if (record.m_kind == ::water::Kind::Line)
      AddLine(record, bounds, result);
    else if (record.m_kind == ::water::Kind::Unsupported)
      result.m_unknownAreas.push_back(record.GetBounds());
  }
  if (!AddSurface(records, bounds, cancelled, result))
    return {};
  result.m_bounds = bounds;
  IndexTriangles(result);
  result.m_complete = true;
  return result;
}

water::Geometry WaterGeometryProvider::ExtractFeatures(DataSource const & dataSource, MwmSet::MwmId const & id,
                                                       m2::RectD const & bounds, water::CancelledFn const & cancelled)
{
  return ExtractFeatures(dataSource, std::vector<MwmSet::MwmId>{id}, bounds, cancelled);
}

water::Geometry WaterGeometryProvider::ExtractFeatures(DataSource const & dataSource,
                                                       std::vector<MwmSet::MwmId> const & ids, m2::RectD const & bounds,
                                                       water::CancelledFn const & cancelled)
{
  return ExtractFeatures(dataSource, ids, bounds, cancelled, {});
}

water::Geometry WaterGeometryProvider::ExtractFeatures(DataSource const & dataSource,
                                                       std::vector<MwmSet::MwmId> const & ids, m2::RectD const & bounds,
                                                       water::CancelledFn const & cancelled,
                                                       std::vector<m2::RectD> const & regions)
{
  water::Geometry result;
  result.m_hasSourceTopology = false;
  std::map<std::pair<int8_t, Point>, uint64_t> endpoints;
  std::vector<FeatureSurface> surfaces;
  size_t points = 0, triangles = 0, features = 0;
  bool exceeded = false;
  for (auto const & id : ids)
  {
    if (cancelled() || exceeded)
    {
      result = {};
      result.m_resourceLimited = exceeded;
      return result;
    }
    dataSource.ForEachInRectForMWM([&](FeatureType & feature)
    {
      if (cancelled() || exceeded)
        return;
      std::vector<uint32_t> types;
      feature.ForEachType([&](uint32_t type) { types.push_back(type); });
      auto const line = feature.GetGeomType() == feature::GeomType::Line && ::water::IsLine(types);
      auto const area = feature.GetGeomType() == feature::GeomType::Area && ::water::IsArea(types);
      if (!line && !area)
        return;
      auto const rect = feature.GetLimitRect(FeatureType::BEST_GEOMETRY);
      if (!rect.IsIntersect(bounds) ||
          (!regions.empty() &&
           std::none_of(regions.begin(), regions.end(), [&](auto const & region) { return rect.IsIntersect(region); })))
        return;
      // Generated coastline cells contain sea triangles with land holes already
      // removed. Read them like inland water; never infer water from empty space.
      // Unrelated buildings and roads must not exhaust the water geometry budget.
      if (++features > water::kMaxWaterFeatures)
      {
        exceeded = true;
        return;
      }
      // Bridge/aqueduct/lock restrictions are not modelled in display-data
      // measurements. Underground features allow a walking bypass.
      bool underground = feature.GetLayer() < 0;
      feature.ForEachType([&](uint32_t type)
      {
        auto const name = classif().GetReadableObjectName(type);
        underground = underground || name.find("tunnel") != std::string::npos;
      });
      if (underground)
      {
        result.m_portageAreas.push_back(rect);
        return;
      }
      if (line)
      {
        auto const & vertices = feature.GetPoints(FeatureType::BEST_GEOMETRY);
        ::water::Record record;
        record.m_kind = ::water::Kind::Line;
        record.m_rings.emplace_back(vertices.begin(), vertices.end());
        record.m_nodes.resize(vertices.size(), 0);
        // Ordinary MWMs omit source node IDs. Recover only exact stored endpoint
        // contacts on the same layer, never crossings, proximity or clipping cuts.
        if (vertices.size() >= 2)
          for (auto const i : {size_t{0}, vertices.size() - 1})
          {
            auto const [it, inserted] =
                endpoints.emplace(std::pair{feature.GetLayer(), vertices[i]}, endpoints.size() + 1);
            record.m_nodes[i] = it->second;
          }
        auto const firstLine = result.m_lines.size();
        if (vertices.size() >= 2)
          AddLine(record, bounds, result);
        // Charge only retained vertices, including clipping intersections. Source
        // vertices outside the window do not consume graph capacity.
        for (size_t i = firstLine; i < result.m_lines.size(); ++i)
        {
          if (result.m_lines[i].size() > water::kMaxGraphVertices - 2 - points)
          {
            exceeded = true;
            return;
          }
          points += result.m_lines[i].size();
        }
        return;
      }
      FeatureSurface surface;
      feature.ForEachTriangle([&](Point const & a, Point const & b, Point const & c)
      {
        if (cancelled() || exceeded)
          return;
        auto const append = [&](m2::RectD const & region)
        {
          auto const clipped = Clip({a, b, c}, region);
          for (size_t i = 2; i < clipped.size(); ++i)
          {
            if (m2::robust::OrientedS(clipped[0], clipped[i - 1], clipped[i]) == 0.0)
              continue;
            if (++triangles > kMaxTriangles)
            {
              exceeded = true;
              return;
            }
            surface.m_triangles.push_back({::water::Kind::Area, {{clipped[0], clipped[i - 1], clipped[i]}}, {}});
          }
        };
        if (regions.empty())
          append(bounds);
        else
        {
          m2::RectD triangleBounds(a, b);
          triangleBounds.Add(c);
          for (auto const & region : regions)
            if (!exceeded && region.IsIntersect(triangleBounds))
              append(region);
        }
      }, FeatureType::BEST_GEOMETRY);
      if (!surface.m_triangles.empty())
        surfaces.push_back(std::move(surface));
    }, bounds, scales::GetUpperScale(), id);
  }
  if (cancelled() || exceeded || !AddFeatureSurfaces(surfaces, bounds, cancelled, result))
  {
    result = {};
    result.m_resourceLimited = !cancelled();
    return result;
  }
  IndexTriangles(result);
  result.m_bounds = bounds;
  result.m_complete = true;
  return result;
}

void WaterGeometryProvider::SetCoverage(water::Geometry & geometry, std::vector<Point> const & checkpoints,
                                        RectCoverageFn covered, water::CancelledFn const & cancelled,
                                        std::vector<m2::RectD> const & regions)
{
  if (!regions.empty())
    covered = [regions, covered = std::move(covered)](m2::RectD const & rect)
    {
      if (!covered(rect))
        return false;
      std::vector<m2::RectD> remaining{rect};
      for (auto const & region : regions)
      {
        std::vector<m2::RectD> next;
        for (auto const & piece : remaining)
        {
          auto cut = piece;
          if (!cut.Intersect(region))
          {
            next.push_back(piece);
            continue;
          }
          if (piece.minX() < cut.minX())
            next.emplace_back(piece.minX(), piece.minY(), cut.minX(), piece.maxY());
          if (piece.maxX() > cut.maxX())
            next.emplace_back(cut.maxX(), piece.minY(), piece.maxX(), piece.maxY());
          if (piece.minY() < cut.minY())
            next.emplace_back(cut.minX(), piece.minY(), cut.maxX(), cut.minY());
          if (piece.maxY() > cut.maxY())
            next.emplace_back(cut.minX(), cut.maxY(), cut.maxX(), piece.maxY());
        }
        remaining = std::move(next);
        if (remaining.empty())
          return true;
      }
      return false;
    };
  CHECK(geometry.m_bounds, ());
  AddCoverageGaps(*geometry.m_bounds, checkpoints, covered, cancelled, geometry.m_coverageGaps);
  geometry.m_coverageIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_coverageGaps.size(); ++i)
    geometry.m_coverageIndex->Add(i, geometry.m_coverageGaps[i]);
  geometry.m_coverageIndex->Optimize();
  geometry.m_isCovered = [covered = std::move(covered), cache = std::map<std::array<double, 4>, bool>{}](
                             Point const & a, Point const & b) mutable
  {
    auto const query = [&](m2::RectD const & rect)
    {
      std::array<double, 4> const key{rect.minX(), rect.minY(), rect.maxX(), rect.maxY()};
      if (auto const it = cache.find(key); it != cache.end())
        return it->second;
      bool const result = covered(rect);
      if (cache.size() == 65536)
        cache.clear();
      cache.emplace(key, result);
      return result;
    };
    auto const certified = [&](m2::RectD const & rect)
    {
      // Neighboring mesh edges reuse certified tiles instead of rechecking the
      // same packed border for every vertex. Uncertified tiles prove nothing.
      for (double const step : {0.01, 0.001})
      {
        auto const x = std::floor(rect.minX() / step) * step;
        auto const y = std::floor(rect.minY() / step) * step;
        m2::RectD const tile(x, y, x + step, y + step);
        if (tile.IsRectInside(rect) && query(tile))
          return true;
      }
      return query(rect);
    };
    size_t visits = 0;
    auto const visit = [&](auto && self, Point const & from, Point const & to, size_t depth) -> bool
    {
      if (++visits > 4096)
        return false;
      if (certified(m2::RectD(from, to)))
        return true;
      auto const middle = (from + to) * 0.5;
      if (depth == 24 || !certified(m2::RectD(from, from)) || !certified(m2::RectD(to, to)) ||
          !certified(m2::RectD(middle, middle)))
        return false;
      return self(self, from, middle, depth + 1) && self(self, middle, to, depth + 1);
    };
    return visit(visit, a, b, 0);
  };
}

std::vector<m2::RectD> WaterGeometryProvider::FindWaterCorridor(std::vector<Point> const & points,
                                                                TileLoader const & loader,
                                                                water::CancelledFn const & cancelled,
                                                                unsigned refinement)
{
  if (points.size() < 2 || cancelled())
    return {};
  m2::RectD pins;
  for (auto const & point : points)
    pins.Add(point);
  auto const extent =
      mercator::RectByCenterXYAndSizeInMeters(pins.Center(), std::ldexp(10000.0, -static_cast<int>(refinement)));
  double const step = std::min(extent.SizeX(), extent.SizeY());
  using Cell = std::tuple<int, int, unsigned>;
  auto const bounds = [step](Cell const & cell)
  {
    auto const [x, y, level] = cell;
    double const size = std::ldexp(step, -static_cast<int>(level));
    return m2::RectD(x * size, y * size, (x + 1) * size, (y + 1) * size);
  };
  struct Tile
  {
    bool m_split = false;
    water::Connectivity m_connectivity;
  };
  size_t loads = 0, decoded = 0, stored = 0;
  bool exhausted = false;
  std::vector<m2::RectD> selected;
  for (size_t leg = 1; leg < points.size(); ++leg)
  {
    std::map<Cell, Tile> tiles;
    // The loading guide is undirected: use the same corridor when reversing a leg.
    std::array<Point, 2> const checkpoints{std::min(points[leg - 1], points[leg]),
                                           std::max(points[leg - 1], points[leg])};
    auto const load = [&](Cell const & cell) -> Tile &
    {
      auto const [it, inserted] = tiles.try_emplace(cell);
      if (!inserted)
        return it->second;
      if (cancelled() || loads >= 1024 || decoded >= 8 * water::kMaxTriangles || stored >= water::kMaxGraphVertices)
      {
        exhausted = true;
        return it->second;
      }
      auto const rect = bounds(cell);
      if (!mercator::Bounds::FullRect().IsRectInside(rect))
        return it->second;
      ++loads;
      auto geometry = loader(rect);
      // Only one tile's geometry is retained at a time; cache compact connectivity instead.
      size_t units = geometry.m_triangles.size();
      for (auto const & line : geometry.m_lines)
        units += line.size();
      decoded += units;
      auto & tile = it->second;
      if ((!geometry.m_complete || units > 32768) && std::get<2>(cell) < 4)
      {
        tile.m_split = true;
        return tile;
      }
      geometry.m_bounds = rect;
      tile.m_connectivity = water::GetConnectivity(geometry, checkpoints, cancelled);
      stored += tile.m_connectivity.m_portals.size();
      return tile;
    };
    std::function<void(Cell const &, m2::RectD const &, std::vector<Cell> &)> visit;
    visit = [&](Cell const & cell, m2::RectD const & query, std::vector<Cell> & leaves)
    {
      if (exhausted || cancelled() || !bounds(cell).IsIntersect(query))
        return;
      auto const & tile = load(cell);
      if (!tile.m_split)
      {
        if (tile.m_connectivity.m_complete)
          leaves.push_back(cell);
        return;
      }
      auto const [x, y, level] = cell;
      for (int dx = 0; dx < 2; ++dx)
        for (int dy = 0; dy < 2; ++dy)
          visit({2 * x + dx, 2 * y + dy, level + 1}, query, leaves);
    };
    auto const leavesFor = [&](m2::RectD const & query)
    {
      std::vector<Cell> leaves;
      for (int x = std::floor(query.minX() / step); x <= std::floor(query.maxX() / step); ++x)
        for (int y = std::floor(query.minY() / step); y <= std::floor(query.maxY() / step); ++y)
          visit({x, y, 0}, query, leaves);
      return leaves;
    };
    using Node = std::pair<Cell, size_t>;
    using Entry = std::pair<double, Node>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
    std::map<Node, double> distances;
    std::map<Node, Node> parents;
    std::set<Node> closed;
    // A boundary pin can belong to water only on the other side of its grid cell.
    // Include all incident cells without widening the actual water membership test.
    auto const & source = checkpoints[0];
    m2::RectD const sourceCells(std::nextafter(source.x, source.x - 1), std::nextafter(source.y, source.y - 1),
                                std::nextafter(source.x, source.x + 1), std::nextafter(source.y, source.y + 1));
    for (auto const & cell : leavesFor(sourceCells))
      if (auto const component = tiles.at(cell).m_connectivity.m_checkpoints[0])
      {
        Node const node{cell, *component};
        distances[node] = 0;
        queue.emplace(0, node);
      }
    std::optional<Node> finish;
    while (!queue.empty() && !exhausted && !cancelled())
    {
      auto const node = queue.top().second;
      queue.pop();
      if (!closed.insert(node).second)
        continue;
      auto const & connectivity = tiles.at(node.first).m_connectivity;
      if (connectivity.m_checkpoints[1] == node.second)
      {
        finish = node;
        break;
      }
      auto const rect = bounds(node.first);
      for (auto const & portal : connectivity.m_portals)
      {
        if (portal.m_component != node.second)
          continue;
        for (size_t side = 0; side < 4; ++side)
        {
          bool const vertical = side < 2;
          double const edge = side == 0 ? rect.minX() : side == 1 ? rect.maxX() : side == 2 ? rect.minY() : rect.maxY();
          auto const coord = [vertical](Point const & p) { return vertical ? p.x : p.y; };
          if (coord(portal.m_from) != edge || coord(portal.m_to) != edge)
            continue;
          auto a = portal.m_from, b = portal.m_to;
          double const outside = std::nextafter(edge, edge + (side % 2 == 0 ? -1.0 : 1.0));
          (vertical ? a.x : a.y) = (vertical ? b.x : b.y) = outside;
          for (auto const & neighbor : leavesFor(m2::RectD(a, b)))
            for (auto const & other : tiles.at(neighbor).m_connectivity.m_portals)
            {
              if (coord(other.m_from) != edge || coord(other.m_to) != edge)
                continue;
              auto const along = [vertical](Point const & p) { return vertical ? p.y : p.x; };
              if (std::max(along(portal.m_from), along(other.m_from)) > std::min(along(portal.m_to), along(other.m_to)))
                continue;
              Node const next{neighbor, other.m_component};
              if (closed.contains(next))
                continue;
              double const cost =
                  distances.at(node) + mercator::DistanceOnEarth(rect.Center(), bounds(neighbor).Center());
              if (auto const it = distances.find(next); it != distances.end() && it->second <= cost)
                continue;
              distances[next] = cost;
              parents[next] = node;
              queue.emplace(cost + mercator::DistanceOnEarth(bounds(neighbor).Center(), checkpoints[1]), next);
            }
        }
      }
    }
    if (!finish)
      return {};
    for (auto node = *finish;; node = parents.at(node))
    {
      if (std::find(selected.begin(), selected.end(), bounds(node.first)) == selected.end())
        selected.push_back(bounds(node.first));
      if (!parents.contains(node))
        break;
    }
  }
  return {selected.begin(), selected.end()};
}

water::Geometry WaterGeometryProvider::ExpandGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate,
                                                      water::Geometry const & previous,
                                                      std::array<bool, 4> const & frontiers, bool adaptive)
{
  // A failed final search requests a finer connectivity corridor, never the same window.
  if (previous.m_selective)
    return previous.m_corridorRefinement < 2
             ? GetGeometry(checkpoints, delegate, 0.0, {}, previous.m_corridorRefinement + 1)
             : water::Geometry{};
  if (!adaptive)
    return GetGeometry(checkpoints, delegate, 2000.0);
  if (!previous.m_bounds)
    return {};
  m2::RectD pins;
  double direct = 0;
  for (auto const & point : checkpoints.GetPoints())
  {
    pins.Add(point);
    direct = std::max(direct, mercator::DistanceOnEarth(checkpoints.GetPoints().front(), point));
  }
  auto const margin = std::min(std::clamp(2 * direct, 1000.0, 30000.0) + 2000.0, 30000.0);
  auto const extent = mercator::RectByCenterXYAndSizeInMeters(pins.Center(), margin);
  auto const & old = *previous.m_bounds;
  auto const grow = [](double margin, double limit) { return std::min(margin * 3, limit); };
  m2::RectD bounds(frontiers[0] ? pins.minX() - grow(pins.minX() - old.minX(), extent.SizeX() / 2) : old.minX(),
                   frontiers[2] ? pins.minY() - grow(pins.minY() - old.minY(), extent.SizeY() / 2) : old.minY(),
                   frontiers[1] ? pins.maxX() + grow(old.maxX() - pins.maxX(), extent.SizeX() / 2) : old.maxX(),
                   frontiers[3] ? pins.maxY() + grow(old.maxY() - pins.maxY(), extent.SizeY() / 2) : old.maxY());
  if (bounds == old)
    return {};
  return GetGeometry(checkpoints, delegate, 0.0, bounds);
}

water::Geometry WaterGeometryProvider::GetGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate,
                                                   double extraMargin, std::optional<m2::RectD> requestedBounds,
                                                   unsigned corridorRefinement)
{
  auto const cancelled = [&] { return delegate.IsCancelled(); };
  auto const & points = checkpoints.GetPoints();
  m2::RectD bounds;
  double direct = 0;
  for (auto const & point : points)
  {
    bounds.Add(point);
    direct = std::max(direct, mercator::DistanceOnEarth(points.front(), point));
  }
  auto const checkpointBounds = bounds;
  // A finite local query supports direct routes and ordinary detours. Larger
  // systems remain unavailable instead of scanning countries or guessing gaps.
  // Point eligibility needs only nearby water, not a full routing window.
  auto const margin = std::min((direct == 0.0 ? 50.0 : std::clamp(2 * direct, 1000.0, 30000.0)) + extraMargin, 30000.0);
  auto const extent = mercator::RectByCenterXYAndSizeInMeters(bounds.Center(), margin);
  bounds.Inflate(extent.SizeX() / 2, extent.SizeY() / 2);
  if (requestedBounds)
    bounds = *requestedBounds;
  if (!mercator::Bounds::FullRect().IsRectInside(bounds) || bounds.SizeX() >= 180)
    return {};
  std::vector<std::shared_ptr<MwmInfo>> infos;
  m_dataSource.GetMwmsInfo(infos);
  std::vector<MwmSet::MwmId> standardMaps;
  // Bound the cache and release readers for replaced map versions.
  for (auto it = m_readers.begin(); it != m_readers.end();)
    if (!it->first.IsAlive())
      it = m_readers.erase(it);
    else
      ++it;
  for (auto const & info : infos)
  {
    if (cancelled())
      return {};
    if (info->GetType() != MwmInfo::COUNTRY || !info->m_bordersRect.IsIntersect(bounds))
      continue;
    MwmSet::MwmId id(info);
    auto handle = m_dataSource.GetMwmHandleById(id);
    if (!handle.IsAlive())
      continue;
    auto const & container = handle.GetValue()->m_cont;
    if (!container.IsExist(::water::kSectionTag))
    {
      standardMaps.push_back(id);
      continue;
    }
    if (!m_readers.contains(id))
      m_readers.clear();
    auto & reader = m_readers[id];
    if (!reader)
      reader = std::make_unique<::water::Reader>(container.GetReader(::water::kSectionTag));
    if (reader->Covers(bounds))
      return Extract(*reader, bounds, cancelled);
  }
  std::vector<std::vector<MwmSet::MwmId>> mapGroups;
  // Prefer combined coverage before clipping a window to a single map.
  if (standardMaps.size() > 1 && standardMaps.size() <= kMaxStandardMaps && m_multiCoverage)
    mapGroups.push_back(standardMaps);
  for (auto const & id : standardMaps)
    mapGroups.push_back({id});
  // Large fixed margins collect unrelated lakes and may cross a map-region
  // boundary. Keep standard-map extraction local; shrink only on extraction failure.
  for (double localMargin = std::min(margin, 3000.0 + extraMargin);; localMargin = std::max(250.0, localMargin * 0.8))
  {
    bounds = checkpointBounds;
    auto const localExtent = mercator::RectByCenterXYAndSizeInMeters(bounds.Center(), localMargin);
    bounds.Inflate(localExtent.SizeX() / 2, localExtent.SizeY() / 2);
    if (requestedBounds)
      bounds = *requestedBounds;
    for (auto const & ids : mapGroups)
    {
      if (cancelled())
        return {};
      std::vector<std::string> countries;
      m2::RectD combinedBounds;
      for (auto const & id : ids)
      {
        auto const info = id.GetInfo();
        countries.push_back(info->GetCountryName());
        combinedBounds.Add(info->m_bordersRect);
      }
      auto const covered =
          [combinedBounds, countries, coverage = m_coverage, multiCoverage = m_multiCoverage](m2::RectD const & rect)
      {
        if (!combinedBounds.IsRectInside(rect))
          return false;
        return countries.size() == 1 ? coverage && coverage(countries.front(), rect)
                                     : multiCoverage && multiCoverage(countries, rect);
      };
      // Checkpoint membership still requires downloaded coverage. Missing parts
      // elsewhere in the query become unknown areas, never water or portages.
      if (!std::all_of(points.begin(), points.end(),
                       [&](auto const & point) { return covered(m2::RectD(point, point)); }))
        continue;
      water::Geometry result;
      if (corridorRefinement == 0)
        result = ExtractFeatures(m_dataSource, ids, bounds, cancelled);
      else
        result.m_resourceLimited = true;
      if (result.m_resourceLimited && !cancelled())
      {
        // Preserve local water components, including rivers, across tile boundaries.
        // Both the guide and the final route respect downloaded-region coverage.
        auto const mapsFor = [&](auto const & regions)
        {
          std::vector<MwmSet::MwmId> selected;
          for (auto const & info : infos)
            if (info->GetType() == MwmInfo::COUNTRY &&
                std::any_of(regions.begin(), regions.end(),
                            [&](auto const & region) { return info->m_bordersRect.IsIntersect(region); }))
              selected.emplace_back(info);
          return selected;
        };
        auto regions = FindWaterCorridor(points, [&](auto const & tileBounds)
        {
          auto const tileMaps = mapsFor(std::array{tileBounds});
          if (tileMaps.size() > kMaxStandardMaps)
            return water::Geometry{};
          auto tile = ExtractFeatures(m_dataSource, tileMaps, tileBounds, cancelled);
          if (tile.m_complete)
          {
            std::vector<std::string> names;
            for (auto const & id : tileMaps)
              names.push_back(id.GetInfo()->GetCountryName());
            SetCoverage(tile, points, [&, names](m2::RectD const & rect)
            {
              return names.size() == 1 ? m_coverage && m_coverage(names.front(), rect)
                                       : !names.empty() && m_multiCoverage && m_multiCoverage(names, rect);
            }, cancelled);
          }
          return tile;
        }, cancelled, corridorRefinement);
        if (regions.empty())
        {
          result.m_corridorUnavailable = !cancelled();
          return result;
        }
        m2::RectD selectedBounds;
        for (auto const & region : regions)
          selectedBounds.Add(region);
        auto const selectedMaps = mapsFor(regions);
        if (selectedMaps.size() > kMaxStandardMaps)
          return result;
        result = ExtractFeatures(m_dataSource, selectedMaps, selectedBounds, cancelled, regions);
        result.m_selective = true;
        result.m_corridorRefinement = corridorRefinement;
        if (result.m_complete)
        {
          // Selected source features keep their original topology. Unloaded
          // regions remain unknown, including to pedestrian transfer candidates.
          std::vector<std::string> selectedCountries;
          m2::RectD selectedMapBounds;
          for (auto const & id : selectedMaps)
          {
            selectedCountries.push_back(id.GetInfo()->GetCountryName());
            selectedMapBounds.Add(id.GetInfo()->m_bordersRect);
          }
          SetCoverage(result, points,
                      [selectedMapBounds, selectedCountries, coverage = m_coverage,
                       multiCoverage = m_multiCoverage](m2::RectD const & rect)
          {
            if (!selectedMapBounds.IsRectInside(rect) || selectedCountries.empty())
              return false;
            return selectedCountries.size() == 1 ? coverage && coverage(selectedCountries.front(), rect)
                                                 : multiCoverage && multiCoverage(selectedCountries, rect);
          },
                      cancelled, regions);
        }
        return result;
      }
      if (result.m_complete)
        SetCoverage(result, points, covered, cancelled);
      if (cancelled())
        return {};
      if (result.m_complete)
        return result;
    }
    if (requestedBounds || localMargin <= 250.0 || cancelled())
      break;
  }
  return {};
}
}  // namespace routing
