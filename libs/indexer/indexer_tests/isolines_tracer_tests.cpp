#include "testing/testing.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_normals.hpp"
#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/twm_set.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <cmath>
#include <map>

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
      TEST(std::isfinite(point.x) && std::isfinite(point.y), (isoline.m_altitude));
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
      TEST(std::isfinite(actual.m_x) && std::isfinite(actual.m_y), (p, node));
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

// A raster patch decodes whole, but a smaller query keeps only its cells around the query.
UNIT_TEST(TerrainReader_RasterPatchCropsToQuery)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  size_t const geomIndex = header.m_geometries.size() - 1;
  terrain::TerrainData full;
  reader.ReadTerrain(header.GetLimitRect(), geomIndex, full);
  TEST(!full.m_rasterPatches.empty(), ());
  std::map<uint64_t, terrain::Altitude> heights;
  for (auto const & patch : full.m_rasterPatches)
    for (uint32_t node = 0; node < patch.m_heights.size(); ++node)
      heights.emplace(terrain::impl::PointKey(patch.PointU(node)), patch.m_heights[node]);

  auto const & whole = full.m_rasterPatches.front();
  auto const wholeRect = whole.GetRect();
  m2::RectD query(wholeRect.Center(), wholeRect.Center());
  query.Inflate(wholeRect.SizeX() / 10, wholeRect.SizeY() / 10);
  terrain::TerrainData data;
  reader.ReadTerrain(query, geomIndex, data);
  TEST_EQUAL(data.m_rasterPatches.size(), 1, ());
  auto const & patch = data.m_rasterPatches.front();
  TEST_LESS(patch.m_heights.size(), whole.m_heights.size(), ());
  TEST_EQUAL(patch.m_heights.size(), patch.m_columns.size() * patch.m_rows.size(), ());
  TEST(patch.GetRect().IsRectInside(query), (patch.GetRect(), query));
  for (uint32_t node = 0; node < patch.m_heights.size(); ++node)
    TEST_EQUAL(patch.m_heights[node], heights.at(terrain::impl::PointKey(patch.PointU(node))), (node));
}

// The decoded heights of the fixture's 4 SPERR and 5 SZ3 patches must match on every platform and survive codec
// updates: a round trip through the vendored encoder misses the changes both sides share. Update the value only
// together with the fixture.
UNIT_TEST(TerrainReader_RealHybridRasterChecksum)
{
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  terrain::TerrainData data;
  reader.ReadTerrain(header.GetLimitRect(), header.m_geometries.size() - 1, data);
  uint64_t hash = 14695981039346656037ULL;  // FNV-1a over every decoded height.
  size_t count = 0;
  for (auto const & patch : data.m_rasterPatches)
  {
    for (auto const height : patch.m_heights)
    {
      hash = (hash ^ static_cast<uint16_t>(height)) * 1099511628211ULL;
      ++count;
    }
  }
  TEST_EQUAL(data.m_rasterPatches.size(), 9, ());
  TEST_EQUAL(count, 130321, ());
  TEST_EQUAL(hash, 4174731811693775522ULL, ());
}

// Registration keeps the scanned file identity and rejects overlapping active blocks.
UNIT_TEST(TerrainTwmSet_Registration)
{
  std::string const source = GetTestBlockPath();
  // Two copies of the same block = the same header rect = a guaranteed overlap.
  std::string const newerPath = base::JoinPath(GetPlatform().TmpDir(), "twm_v2.twm");
  std::string const olderPath = base::JoinPath(GetPlatform().TmpDir(), "twm_v1.twm");
  TEST(base::CopyFileX(source, newerPath), ());
  TEST(base::CopyFileX(source, olderPath), ());
  SCOPE_GUARD(cleanup, [&]()
  {
    base::DeleteFileX(newerPath);
    base::DeleteFileX(olderPath);
  });

  terrain::TwmFile newer, older;
  TEST_EQUAL(terrain::TwmSet::ReadFile(newerPath, 2 /* version */, newer), terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(terrain::TwmSet::ReadFile(olderPath, 1 /* version */, older), terrain::TwmSet::RegResult::Success, ());
  TEST(newer.m_rect.IsValid(), ());

  terrain::TwmSet set;
  auto const [id, result] = set.Register(newer);
  TEST_EQUAL(result, terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(set.GetId(newer.m_id), id, ());
  TEST_EQUAL(id.GetInfo()->GetFilePath(), newerPath, ());
  TEST_EQUAL(id.GetInfo()->GetVersion(), 2, ());
  TEST_EQUAL(set.Register(older).second, terrain::TwmSet::RegResult::Overlapping, ());
  TEST(set.HasBlocks(newer.m_rect), ());

  set.Deregister(newer.m_id);
  TEST(!set.GetId(newer.m_id).IsAlive(), ());
  TEST_EQUAL(set.Register(older).second, terrain::TwmSet::RegResult::Success, ());
  TEST_EQUAL(set.GetId(older.m_id).GetInfo()->GetVersion(), 1, ());
}
}  // namespace isolines_tracer_tests
