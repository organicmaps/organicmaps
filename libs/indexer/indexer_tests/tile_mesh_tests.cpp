#include "testing/testing.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_data.hpp"
#include "indexer/terrain/tile_mesh.hpp"

#include "coding/point_coding.hpp"

#include <map>
#include <vector>

namespace tile_mesh_tests
{
using measurement_utils::Units;

// The cross-feature vertex deduplication is the mechanism every terrain consumer rests
// on: the shade smooths the normals per shared vertex, and the isoline chains continue
// across features only through the merged adjacency. Two CCW triangles sharing the
// diagonal of a unit square arrive as separate features (6 vertices), the mesh must
// collapse them into one indexed mesh (4 vertices), and every traced level must walk
// through the shared diagonal as ONE chain - a broken merge fragments it into two.
UNIT_TEST(TileMesh_DedupAndTrace)
{
  // The square A(0,0)=0m B(1,0)=90m C(1,1)=180m D(0,1)=90m in mercator, split by the
  // A-C diagonal. No traced level equals a vertex altitude, so the crossings stay
  // strictly inside the edges in both units.
  auto const q = [](double x, double y) { return PointDToPointU({x, y}, terrain::kTerrainCoordBits); };

  terrain::TileMesh mesh;
  // The first feature.
  uint32_t const a = mesh.AddVertex(q(0.0, 0.0), 0);
  uint32_t const b = mesh.AddVertex(q(1.0, 0.0), 90);
  uint32_t const c = mesh.AddVertex(q(1.0, 1.0), 180);
  mesh.AddTriangle(a, b, c);
  // The second one re-adds the shared A and C: the same indices come back.
  TEST_EQUAL(mesh.AddVertex(q(0.0, 0.0), 0), a, ());
  TEST_EQUAL(mesh.AddVertex(q(1.0, 1.0), 180), c, ());
  uint32_t const d = mesh.AddVertex(q(0.0, 1.0), 90);
  mesh.AddTriangle(a, c, d);

  // 6 input vertices collapse into 4, the A and C instances are shared.
  TEST_EQUAL(mesh.GetPoints().size(), 4, ());
  TEST_EQUAL(mesh.GetAltitudes().size(), 4, ());
  TEST_EQUAL(mesh.GetTrianglesCount(), 2, ());

  struct Expectation
  {
    Units m_units;
    int32_t m_step;
    std::vector<int32_t> m_levels;
  };
  // The metric levels cross the 0..180 m altitudes; the imperial vertex altitudes are
  // lround(MetersToFeet): 0, 295, 591 ft, so the 100 ft levels are 100..500.
  std::vector<Expectation> const expectations = {
      {Units::Metric, 50, {50, 100, 150}},
      {Units::Imperial, 100, {100, 200, 300, 400, 500}},
  };

  for (auto const & e : expectations)
  {
    std::map<int32_t, size_t> chains;
    terrain::TraceIsolines(mesh, e.m_step, e.m_units, [&](terrain::Isoline && isoline)
    {
      ++chains[isoline.m_altitude];
      // Every level crosses both triangles through the shared diagonal: one OPEN chain
      // of exactly 3 crossing points (two border edges + the diagonal).
      TEST(!isoline.m_closed, (isoline.m_altitude));
      TEST_EQUAL(isoline.m_points.size(), 3, (isoline.m_altitude));
    });
    TEST_EQUAL(chains.size(), e.m_levels.size(), (static_cast<int>(e.m_units)));
    for (int32_t const level : e.m_levels)
      TEST_EQUAL(chains[level], 1, (static_cast<int>(e.m_units), level));
  }
}
UNIT_TEST(TerrainData_ElevationTin)
{
  terrain::TerrainData data;
  TEST(!data.GetElevation({0, 0}), ());
  auto const q = [](double x, double y) { return PointDToPointU({x, y}, terrain::kTerrainCoordBits); };
  auto & mesh = data.m_mesh;
  auto const a = mesh.AddVertex(q(0, 0), -100);
  auto const b = mesh.AddVertex(q(1, 0), 100);
  auto const c = mesh.AddVertex(q(0, 1), 100);
  mesh.AddTriangle(a, b, c);
  auto const & points = mesh.GetPoints();
  auto const at = [&](double u, double v)
  { return data.GetElevation(points[a] * (1.0 - u - v) + points[b] * u + points[c] * v); };
  TEST_ALMOST_EQUAL_ABS(*at(0, 0), -100.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(*at(0.25, 0.25), 0.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(*at(0.25, 0.5), 50.0, 1e-8, ());
  TEST_ALMOST_EQUAL_ABS(*at(0.5, 0.5), 100.0, 1e-8, ("Triangle edge"));
  TEST(!at(0.75, 0.75), ("Inside the bounding box, outside the triangle"));
}

UNIT_TEST(TerrainData_ElevationRaster)
{
  terrain::TerrainData data;
  terrain::RasterPatch patch;
  // Nonuniform cells, with a nonplanar first cell to distinguish the fixed
  // triangulation from bilinear interpolation.
  patch.m_columns = {500000000, 500001000, 500003000};
  patch.m_rows = {500000000, 500002000};
  patch.m_heights = {0, 100, 200, 200, 0, 300};
  data.m_rasterPatches.push_back(patch);
  auto const sw = patch.Point(0), ne = patch.Point(4);
  auto const at = [&](double u, double v)
  { return data.GetElevation({sw.x + (ne.x - sw.x) * u, sw.y + (ne.y - sw.y) * v}); };
  TEST_ALMOST_EQUAL_ABS(*at(0.75, 0.25), 50.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(*at(0.25, 0.75), 100.0, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(*at(0.5, 0.5), 0.0, 1e-6, ("SW-NE diagonal"));
  TEST_ALMOST_EQUAL_ABS(*at(1, 0.5), 50.0, 1e-6, ("Shared cell edge"));
  TEST_ALMOST_EQUAL_ABS(*data.GetElevation(patch.Point(5)), 300.0, 1e-8, ("Outer corner"));
  TEST(!at(-0.01, 0.5), ());
  TEST(!at(3.01, 0.5), ());

  // Splitting the cells into neighboring patches must preserve the same surface
  // and the shared edge, including when the first patch is absent.
  data.m_rasterPatches[0].m_columns.resize(2);
  data.m_rasterPatches[0].m_heights = {0, 100, 200, 0};
  patch.m_columns.erase(patch.m_columns.begin());
  patch.m_heights = {100, 200, 0, 300};
  data.m_rasterPatches.push_back(patch);
  TEST_ALMOST_EQUAL_ABS(*at(1, 0.5), 50.0, 1e-6, ("Shared patch edge"));
  TEST_ALMOST_EQUAL_ABS(*at(2, 0.5), 200.0, 1e-6, ());
  data.m_rasterPatches.erase(data.m_rasterPatches.begin());
  TEST_ALMOST_EQUAL_ABS(*at(1, 0.5), 50.0, 1e-6, ());
  TEST(!at(0.5, 0.5), ());

  auto & mesh = data.m_mesh;
  auto const a = mesh.AddVertex({500000000, 500000000}, -10);
  auto const b = mesh.AddVertex({500001000, 500000000}, 10);
  auto const c = mesh.AddVertex({500000000, 500002000}, 30);
  mesh.AddTriangle(a, b, c);
  TEST_ALMOST_EQUAL_ABS(*at(0.25, 0.25), 5.0, 1e-6, ("TIN next to raster coverage"));
  TEST(!at(0.75, 0.75), ());
}

}  // namespace tile_mesh_tests
