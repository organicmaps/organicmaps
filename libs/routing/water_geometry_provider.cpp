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
#include <memory>

namespace routing
{
namespace
{
using Point = m2::PointD;
size_t constexpr kMaxInputPoints = 16384;
size_t constexpr kMaxSurfacePoints = 4096;
size_t constexpr kMaxTriangles = 16384;

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
      if (ring.size() < 3 || m2::robust::CheckPolygonSelfIntersections(ring.begin(), ring.end()))
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
  geometry.m_triangleIndex = std::make_shared<m4::Tree<size_t>>();
  for (size_t i = 0; i < geometry.m_triangles.size(); ++i)
  {
    m2::RectD rect;
    for (auto const & point : geometry.m_triangles[i])
      rect.Add(point);
    geometry.m_triangleIndex->Add(i, rect);
  }
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
  water::Geometry result;
  result.m_hasSourceTopology = false;
  std::vector<::water::Record> surfaces;
  size_t points = 0, triangles = 0, features = 0;
  bool exceeded = false;
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
    if (!rect.IsIntersect(bounds))
      return;
    // Unrelated buildings and roads must not exhaust the water geometry budget.
    if (++features > 16384)
    {
      exceeded = true;
      return;
    }
    // Coastline polygons encode land. Bridge/aqueduct/lock restrictions are not
    // modelled in display-data measurements; underground waterways stay unknown.
    bool underground = feature.GetLayer() < 0;
    feature.ForEachType([&](uint32_t type)
    {
      auto const name = classif().GetReadableObjectName(type);
      underground = underground || name.find("tunnel") != std::string::npos;
    });
    if (::water::IsCoastline(types) || underground)
    {
      result.m_unknownAreas.push_back(rect);
      return;
    }
    if (line)
    {
      auto const & vertices = feature.GetPoints(FeatureType::BEST_GEOMETRY);
      if (vertices.size() > kMaxInputPoints - points)
      {
        exceeded = true;
        return;
      }
      points += vertices.size();
      ::water::Record record;
      record.m_kind = ::water::Kind::Line;
      record.m_rings.emplace_back(vertices.begin(), vertices.end());
      // Never infer node identity from equal quantized coordinates.
      record.m_nodes.resize(vertices.size(), 0);
      if (vertices.size() >= 2)
        AddLine(record, bounds, result);
      return;
    }
    feature.ForEachTriangle([&](Point const & a, Point const & b, Point const & c)
    {
      if (cancelled() || exceeded)
        return;
      auto const clipped = Clip({a, b, c}, bounds);
      for (size_t i = 2; i < clipped.size(); ++i)
      {
        if (++triangles > kMaxTriangles)
        {
          exceeded = true;
          return;
        }
        surfaces.push_back({::water::Kind::Area, {{clipped[0], clipped[i - 1], clipped[i]}}, {}});
      }
    }, FeatureType::BEST_GEOMETRY);
  }, bounds, scales::GetUpperScale(), id);
  if (cancelled() || exceeded || !AddSurface(surfaces, bounds, cancelled, result, 3 * kMaxTriangles))
    return {};
  IndexTriangles(result);
  result.m_bounds = bounds;
  result.m_complete = true;
  return result;
}

water::Geometry WaterGeometryProvider::GetGeometry(Checkpoints const & checkpoints, RouterDelegate const & delegate)
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
  auto const margin = direct == 0.0 ? 50.0 : std::clamp(2 * direct, 1000.0, 30000.0);
  auto const extent = mercator::RectByCenterXYAndSizeInMeters(bounds.Center(), margin);
  bounds.Inflate(extent.SizeX() / 2, extent.SizeY() / 2);
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
  // Large fixed margins collect unrelated lakes and may cross a map-region
  // boundary. Keep standard-map extraction local; shrink only on extraction failure.
  for (double localMargin = std::min(margin, 3000.0);; localMargin = std::max(250.0, localMargin * 0.5))
  {
    bounds = checkpointBounds;
    auto const localExtent = mercator::RectByCenterXYAndSizeInMeters(bounds.Center(), localMargin);
    bounds.Inflate(localExtent.SizeX() / 2, localExtent.SizeY() / 2);
    for (auto const & id : standardMaps)
    {
      if (cancelled())
        return {};
      auto const info = id.GetInfo();
      auto const covered = [&](m2::RectD const & rect)
      { return info->m_bordersRect.IsRectInside(rect) && m_coverage && m_coverage(info->GetCountryName(), rect); };
      auto mapBounds = bounds;
      if (!covered(mapBounds))
      {
        if (!covered(checkpointBounds))
          continue;
        mapBounds = checkpointBounds;
        // Near a region boundary, expand each side only as far as its coverage
        // permits. A nearby coast must not prevent extraction in other directions.
        for (size_t side = 0; side < 4; ++side)
        {
          auto const origin = mapBounds;
          double low = 0, high = 1;
          for (size_t step = 0; step < 10; ++step)
          {
            if (cancelled())
              return {};
            auto const t = (low + high) * 0.5;
            auto const interpolate = [t](double a, double b) { return a + (b - a) * t; };
            m2::RectD const candidate(side == 0 ? interpolate(origin.minX(), bounds.minX()) : origin.minX(),
                                      side == 1 ? interpolate(origin.minY(), bounds.minY()) : origin.minY(),
                                      side == 2 ? interpolate(origin.maxX(), bounds.maxX()) : origin.maxX(),
                                      side == 3 ? interpolate(origin.maxY(), bounds.maxY()) : origin.maxY());
            if (covered(candidate))
            {
              low = t;
              mapBounds = candidate;
            }
            else
              high = t;
          }
        }
      }
      if (!covered(mapBounds))
        continue;
      auto result = ExtractFeatures(m_dataSource, id, mapBounds, cancelled);
      if (result.m_complete)
        return result;
    }
    if (localMargin <= 250.0 || cancelled())
      break;
  }
  return {};
}
}  // namespace routing
