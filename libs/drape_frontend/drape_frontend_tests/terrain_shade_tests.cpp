#include "testing/testing.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/terrain_shade_shape.hpp"

#include "coding/point_coding.hpp"

#include <algorithm>
#include <cmath>

namespace terrain_shade_tests
{
using gpu::TerrainShadeVertex;

m2::PointD ToGlobal(TerrainShadeVertex const & v, m2::RectD const & tileRect)
{
  return m2::PointD(v.m_position.x, v.m_position.y) / df::kShapeCoordScalar + tileRect.Center();
}

double Cross(m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
{
  return m2::CrossProduct(b - a, c - a);
}

void TestInside(TerrainShadeVertex const & v, m2::RectD const & tileRect)
{
  auto rect = tileRect;
  rect.Inflate(1e-6, 1e-6);
  TEST(rect.IsPointInside(ToGlobal(v, tileRect)), (ToGlobal(v, tileRect)));
  // The flat ground faces up.
  TEST_ALMOST_EQUAL_ABS(v.m_normal.z, 1.0f, 1e-5f, ());
}

// Returns the area of the CW triangles, all inside the tile.
double TestTriangles(std::vector<TerrainShadeVertex> const & vertices, m2::RectD const & tileRect)
{
  TEST_EQUAL(vertices.size() % 3, 0, ());
  double area = 0;
  for (size_t i = 0; i < vertices.size(); i += 3)
  {
    for (size_t j = i; j < i + 3; ++j)
      TestInside(vertices[j], tileRect);
    double const cross = Cross(ToGlobal(vertices[i], tileRect), ToGlobal(vertices[i + 1], tileRect),
                               ToGlobal(vertices[i + 2], tileRect));
    TEST_LESS(cross, 0.0, ("The drape winding is CW", i));
    area -= cross / 2;
  }
  return area;
}

UNIT_TEST(TerrainShade_TinTrianglesClipToTheTileInCwOrder)
{
  m2::RectD const tileRect(0, 0, 1, 1);
  terrain::TerrainData data;
  auto & mesh = data.m_mesh;
  auto const vertex = [&](double x, double y)
  { return mesh.AddVertex(PointDToPointU({x, y}, terrain::kTerrainCoordBits), 0); };
  // CCW: the corner part x + y <= 1 of the tile, a triangle inside it, and a degenerate one.
  mesh.AddTriangle(vertex(-0.5, -0.5), vertex(1.5, -0.5), vertex(-0.5, 1.5));
  mesh.AddTriangle(vertex(0.6, 0.6), vertex(0.9, 0.6), vertex(0.6, 0.9));
  mesh.AddTriangle(vertex(0.6, 0.65), vertex(0.7, 0.75), vertex(0.8, 0.85));
  // Outside the tile.
  mesh.AddTriangle(vertex(2, 2), vertex(3, 2), vertex(2, 3));

  auto const geometry = df::BuildTerrainShade(data, tileRect, {});
  TEST_ALMOST_EQUAL_ABS(TestTriangles(geometry.m_triangles, tileRect), 0.5 + 0.045, 1e-6, ());

  base::SharedCancellable cancelled;
  cancelled.Cancel();
  TEST(df::BuildTerrainShade(data, tileRect, cancelled).m_triangles.empty(), ());
}
}  // namespace terrain_shade_tests
