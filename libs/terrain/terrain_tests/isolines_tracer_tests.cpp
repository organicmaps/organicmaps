#include "testing/testing.hpp"

#include "terrain/isolines_tracer.hpp"
#include "terrain/terrain_normals.hpp"
#include "terrain/terrain_reader.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

namespace isolines_tracer_tests
{
std::string GetTestBlockPath()
{
  return base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
}

// The real Nice fixture includes TIN, SPERR and SZ3 raster terrain. Every open
// contour must end at the block boundary, including contours crossing their seams.
UNIT_TEST(TerrainIsolines_RealHybridInvariants)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  auto const rect = header.GetLimitRect();
  size_t const geomIndex = header.m_geometries.size() - 1;
  terrain::TerrainData data;
  reader.ReadTerrain(rect, geomIndex, data);
  TEST(!data.m_mesh.IsEmpty(), ());
  TEST(!data.m_rasterPatches.empty(), ());

  auto const onBoundary = [&](m2::PointD const & point)
  {
    return std::min({std::abs(point.x - rect.minX()), std::abs(point.x - rect.maxX()), std::abs(point.y - rect.minY()),
                     std::abs(point.y - rect.maxY())}) < 1e-6;
  };
  size_t lines = 0, open = 0, closed = 0;
  terrain::TraceIsolines(data, 100, measurement_utils::Units::Metric, [&](terrain::Isoline && isoline)
  {
    ++lines;
    TEST_EQUAL(isoline.m_altitude % 100, 0, ());
    auto const & points = isoline.m_points;
    TEST_GREATER_OR_EQUAL(points.size(), 2, (isoline.m_altitude));
    if (isoline.m_closed)
    {
      ++closed;
      TEST_GREATER_OR_EQUAL(points.size(), 4, (isoline.m_altitude));
      TEST_EQUAL(points.front(), points.back(), (isoline.m_altitude));
    }
    else
    {
      ++open;
      TEST(onBoundary(points.front()), (isoline.m_altitude, points.front()));
      TEST(onBoundary(points.back()), (isoline.m_altitude, points.back()));
    }
    for (auto const & point : points)
    {
      TEST(math::is_finite(point.x) && math::is_finite(point.y), (isoline.m_altitude));
      TEST(rect.IsPointInside(point), (isoline.m_altitude, point));
    }
  });
  TEST_GREATER(lines, 0, ());
  TEST_GREATER(open, 0, ());
  TEST_GREATER(closed, 0, ());

  base::SharedCancellable cancellation;
  cancellation.Cancel();
  terrain::TerrainData cancelled;
  reader.ReadTerrain(rect, geomIndex, cancelled, cancellation);
  TEST(cancelled.IsEmpty(), ());
}

// A steep triangle crossing over a thousand levels is valid data: it must trace in both units.
UNIT_TEST(TerrainIsolines_SteepTriangle)
{
  terrain::TileMesh mesh;
  auto const a = mesh.AddVertex({1000, 1000}, 0);
  auto const b = mesh.AddVertex({3000, 1000}, 7000);
  auto const c = mesh.AddVertex({1000, 3000}, 0);
  mesh.AddTriangle(a, b, c);
  for (auto const units : {measurement_utils::Units::Metric, measurement_utils::Units::Imperial})
  {
    int32_t const step = units == measurement_utils::Units::Metric ? 10 : 20;
    size_t lines = 0;
    terrain::TraceIsolines(mesh, step, units, [&](terrain::Isoline && isoline)
    {
      TEST_EQUAL(isoline.m_altitude % step, 0, ());
      ++lines;
    });
    // 7000 m is 700 levels at 10 m and 1148 levels at 20 ft.
    TEST_GREATER(lines, units == measurement_utils::Units::Metric ? 690 : 1140, (lines));
  }
}

UNIT_TEST(TerrainIsolines_SplitGridContours)
{
  auto const trace = [](terrain::TerrainData const & data, measurement_utils::Units units)
  {
    std::map<int32_t, std::pair<bool, std::vector<m2::PointD>>> lines;
    terrain::TraceIsolines(data, 20, units, [&](terrain::Isoline && line)
    {
      if (line.m_closed)
      {
        auto & points = line.m_points;
        points.pop_back();
        std::rotate(points.begin(), std::min_element(points.begin(), points.end()), points.end());
        points.push_back(points.front());
      }
      // Each surface has one contour per level; a broken seam must not split it into fragments.
      TEST(lines.emplace(line.m_altitude, std::make_pair(line.m_closed, std::move(line.m_points))).second,
           (line.m_altitude));
    });
    return lines;
  };
  for (auto const & heights : {std::vector<terrain::Altitude>{0, 0, 0, 0, 40, 0, 0, 0, 0},
                               std::vector<terrain::Altitude>{-20, -10, 0, -10, 0, 10, 0, 10, 20}})
  {
    terrain::RasterPatch const whole{{1000000, 2000000, 3000000}, {1000000, 2000000, 3000000}, heights};
    terrain::TerrainData full;
    full.m_rasterPatches.push_back(whole);
    terrain::TerrainData split;
    for (size_t y = 0; y < 2; ++y)
      for (size_t x = 0; x < 2; ++x)
        split.m_rasterPatches.push_back(
            {{whole.m_columns[x], whole.m_columns[x + 1]},
             {whole.m_rows[y], whole.m_rows[y + 1]},
             {heights[3 * y + x], heights[3 * y + x + 1], heights[3 * (y + 1) + x], heights[3 * (y + 1) + x + 1]}});
    auto hybrid = split;
    auto const & corner = hybrid.m_rasterPatches.front();
    for (uint32_t v = 0; v < 4; ++v)
      hybrid.m_mesh.AddVertex(corner.PointU(v), corner.m_heights[v]);
    hybrid.m_mesh.AddTriangle(0, 1, 3);
    hybrid.m_mesh.AddTriangle(0, 3, 2);
    hybrid.m_rasterPatches.erase(hybrid.m_rasterPatches.begin());
    for (auto const units : {measurement_utils::Units::Metric, measurement_utils::Units::Imperial})
    {
      auto const expected = trace(full, units);
      TEST(!expected.empty(), ());
      TEST_EQUAL(trace(split, units), expected, (static_cast<int>(units), "Raster seams"));
      TEST_EQUAL(trace(hybrid, units), expected, (static_cast<int>(units), "TIN/raster seams"));
    }
  }
}

// The normals walk the cells directly: they must sum the very faces of RasterPatch::Triangle.
UNIT_TEST(TerrainNormals_RasterFacesMatchPatchTriangles)
{
  terrain::TerrainData data;
  auto & patch = data.m_rasterPatches.emplace_back();
  patch.m_columns = {1000000, 2000000, 3500000};
  patch.m_rows = {5000000, 5500000, 7000000, 9000000};
  patch.m_heights = {100, 120, 90, 140, 200, 150, 160, 80, 170, 110, 60, 130};
  double constexpr kZScale = 1e-5;

  std::vector<terrain::TerrainNormal> expected(patch.m_heights.size());
  for (uint32_t t = 0; t < 2 * patch.CellsX() * patch.CellsY(); ++t)
  {
    auto const v = patch.Triangle(t);
    auto const ab = patch.Point(v[1]) - patch.Point(v[0]), ac = patch.Point(v[2]) - patch.Point(v[0]);
    double const dzb = (patch.m_heights[v[1]] - patch.m_heights[v[0]]) * kZScale;
    double const dzc = (patch.m_heights[v[2]] - patch.m_heights[v[0]]) * kZScale;
    terrain::TerrainNormal const n = {ab.y * dzc - dzb * ac.y, dzb * ac.x - ab.x * dzc, ab.x * ac.y - ab.y * ac.x};
    for (auto const node : v)
      expected[node] += n;
  }

  auto const normals = terrain::CalculateTerrainNormals(data, kZScale);
  TEST_EQUAL(normals.m_raster.size(), 1, ());
  for (size_t node = 0; node < expected.size(); ++node)
  {
    TEST_ALMOST_EQUAL_ABS(normals.m_raster[0][node].m_x, expected[node].m_x, 1e-12, (node));
    TEST_ALMOST_EQUAL_ABS(normals.m_raster[0][node].m_y, expected[node].m_y, 1e-12, (node));
    TEST_ALMOST_EQUAL_ABS(normals.m_raster[0][node].m_z, expected[node].m_z, 1e-12, (node));
  }
}

UNIT_TEST(TerrainNormals_DrawRectKeepsDrawnNodes)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  terrain::TerrainData data;
  reader.ReadTerrain(header.GetLimitRect(), header.m_geometries.size() - 1, data);
  TEST(!data.m_rasterPatches.empty(), ());
  auto const full = terrain::CalculateTerrainNormals(data, 1.0);

  // On the cell borders: the cells touching a tile edge are drawn too.
  auto const & middle = data.m_rasterPatches.front();
  uint32_t const stride = middle.CellsX() + 1;
  m2::RectD const drawRect(middle.Point(middle.CellsY() / 3 * stride + middle.CellsX() / 3),
                           middle.Point(middle.CellsY() / 2 * stride + middle.CellsX() / 2));
  auto const limited = terrain::CalculateTerrainNormals(data, 1.0, {}, drawRect);

  auto const same = [](terrain::TerrainNormal const & a, terrain::TerrainNormal const & b)
  { return a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z; };
  for (size_t v = 0; v < full.m_tin.size(); ++v)
    TEST(same(limited.m_tin[v], full.m_tin[v]), (v));

  size_t skipped = 0;
  for (size_t p = 0; p < data.m_rasterPatches.size(); ++p)
  {
    auto const & patch = data.m_rasterPatches[p];
    for (auto const & normal : limited.m_raster[p])
      skipped += normal.m_z == 0;
    terrain::RasterPerimeter const perimeter(patch.CellsX(), patch.CellsY());
    for (uint32_t i = 0; i < perimeter.Size(); ++i)
      TEST(same(limited.m_raster[p][perimeter(i)], full.m_raster[p][perimeter(i)]), (p, i));
    uint32_t const cols = patch.CellsX() + 1;
    for (uint32_t y = 0; y < patch.CellsY(); ++y)
    {
      for (uint32_t x = 0; x < patch.CellsX(); ++x)
      {
        uint32_t const sw = y * cols + x, ne = sw + cols + 1;
        if (!drawRect.IsIntersect(m2::RectD(patch.Point(sw), patch.Point(ne))))
          continue;
        for (uint32_t const node : {sw, sw + 1, ne - 1, ne})
          TEST(same(limited.m_raster[p][node], full.m_raster[p][node]), (p, node));
      }
    }
  }
  TEST_GREATER(skipped, 0, ());
}

UNIT_TEST(TerrainNormals_RealHybridInterfaces)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  terrain::TerrainData data;
  reader.ReadTerrain(header.GetLimitRect(), header.m_geometries.size() - 1, data);
  auto const normals = terrain::CalculateTerrainNormals(data, 1.0);
  TEST_EQUAL(normals.m_tin.size(), data.m_mesh.GetPoints().size(), ());
  TEST_EQUAL(normals.m_raster.size(), data.m_rasterPatches.size(), ());

  std::map<uint64_t, terrain::TerrainNormal> interface;
  for (size_t v = 0; v < normals.m_tin.size(); ++v)
    interface.emplace(terrain::impl::PointKey(data.m_mesh.GetPointU(v)), normals.m_tin[v]);
  size_t shared = 0;
  for (size_t p = 0; p < data.m_rasterPatches.size(); ++p)
  {
    auto const & patch = data.m_rasterPatches[p];
    TEST_EQUAL(normals.m_raster[p].size(), patch.m_heights.size(), ());
    terrain::RasterPerimeter const perimeter(patch.CellsX(), patch.CellsY());
    for (uint32_t i = 0; i < perimeter.Size(); ++i)
    {
      auto const node = perimeter(i);
      auto const & actual = normals.m_raster[p][node];
      TEST(math::is_finite(actual.m_x) && math::is_finite(actual.m_y), (p, node));
      TEST_GREATER(actual.m_z, 0.0, (p, node));
      auto const [it, inserted] = interface.emplace(terrain::impl::PointKey(patch.PointU(node)), actual);
      if (!inserted)
      {
        ++shared;
        TEST_EQUAL(actual.m_x, it->second.m_x, (p, node));
        TEST_EQUAL(actual.m_y, it->second.m_y, (p, node));
        TEST_EQUAL(actual.m_z, it->second.m_z, (p, node));
      }
    }
  }
  TEST_GREATER(shared, 0, ("Exercise normals shared by TIN and raster patch perimeters"));
}

}  // namespace isolines_tracer_tests
