#include "testing/testing.hpp"

#include "indexer/terrain/isolines_tracer.hpp"
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
}  // namespace tile_mesh_tests
