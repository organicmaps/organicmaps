#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
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
  TEST(geometry.m_gridRows.empty(), ());
  TEST_ALMOST_EQUAL_ABS(TestTriangles(geometry.m_triangles, tileRect), 0.5 + 0.045, 1e-6, ());

  base::SharedCancellable cancelled;
  cancelled.Cancel();
  TEST(df::BuildTerrainShade(data, tileRect, cancelled).m_triangles.empty(), ());
}

UNIT_TEST(TerrainShade_RasterStripsInsideAndClippedEdgeCells)
{
  m2::RectD const tileRect(0.1, 0.1, 0.9, 0.9);
  terrain::TerrainData data;
  terrain::RasterPatch patch;
  for (double const c : {0.0, 0.25, 0.5, 0.75, 1.0, 1.25})
  {
    patch.m_columns.push_back(PointDToPointU({c, c}, terrain::kTerrainCoordBits).x);
    patch.m_rows.push_back(PointDToPointU({c, c}, terrain::kTerrainCoordBits).y);
  }
  patch.m_heights.assign(patch.m_columns.size() * patch.m_rows.size(), 0);
  data.m_rasterPatches.push_back(std::move(patch));

  auto const geometry = df::BuildTerrainShade(data, tileRect, {});
  // The 2x2 cells inside the tile: the duplicated NW start and then NE, SE per cell.
  TEST_EQUAL(geometry.m_gridRows.size(), 2, ());
  double stripArea = 0;
  for (auto const & row : geometry.m_gridRows)
  {
    TEST_EQUAL(row.size(), 3 + 2 * 2, ());
    for (auto const & v : row)
      TestInside(v, tileRect);
    for (size_t i = 0; i + 2 < row.size(); ++i)
      stripArea +=
          std::abs(Cross(ToGlobal(row[i], tileRect), ToGlobal(row[i + 1], tileRect), ToGlobal(row[i + 2], tileRect))) /
          2;
  }
  TEST_ALMOST_EQUAL_ABS(stripArea, 0.25, 1e-6, ());
  // The ring of the edge cells is clipped to the tile.
  TEST_ALMOST_EQUAL_ABS(TestTriangles(geometry.m_triangles, tileRect), 0.64 - 0.25, 1e-6, ());
}

UNIT_TEST(TerrainShade_IndexedGridMatchesFixedDiagonalTriangles)
{
  uint32_t constexpr kX = 128, kY = 25;
  auto const vertex = [](uint32_t x, uint32_t y)
  {
    // Nonuniform axes and nonlinear per-node lighting make the diagonal observable.
    float const px = -110.0f + 220.0f * x / kX;
    float const py = -100.0f + 200.0f * y * y / (kY * kY);
    glsl::vec3 const normal(0.6f * std::sin(x * 0.7f), 0.6f * std::cos(y * 0.8f), 1.0f);
    return gpu::TerrainShadeVertex(glsl::vec3(px * df::kShapeCoordScalar, py * df::kShapeCoordScalar, 0), normal);
  };
  df::test_support::ShapeTestFixture reference, grid;
  reference.Render("Terrain triangles", 256, 256, [&](auto & fixture)
  {
    std::vector<gpu::TerrainShadeVertex> vertices;
    for (uint32_t y = 0; y < kY; ++y)
      for (uint32_t x = 0; x < kX; ++x)
      {
        auto const sw = vertex(x, y), se = vertex(x + 1, y), ne = vertex(x + 1, y + 1), nw = vertex(x, y + 1);
        vertices.insert(vertices.end(), {sw, ne, se, sw, nw, ne});
      }
    fixture.AddShape(make_unique_dp<df::TerrainShadeShape>(std::move(vertices)));
  });
  grid.Render("Terrain grid", 256, 256, [&](auto & fixture)
  {
    std::vector<std::vector<gpu::TerrainShadeVertex>> rows;
    for (uint32_t y = 0; y < kY; ++y)
    {
      auto & row = rows.emplace_back();
      row.push_back(vertex(0, y + 1));
      for (uint32_t x = 0; x <= kX; ++x)
      {
        row.push_back(vertex(x, y + 1));
        row.push_back(vertex(x, y));
      }
    }
    // 259 vertices and 771 indices per row force several 5000-entry buffer
    // changes in the fixture; splitting a row would change its winding parity.
    fixture.AddShape(make_unique_dp<df::TerrainGridShadeShape>(std::move(rows)));
  });
  auto const & expected = reference.GetLastImage();
  auto const & actual = grid.GetLastImage();
  if (expected.isNull() || actual.isNull())
    return;
  TEST(actual.size() == expected.size(), ());
  size_t shaded = 0;
  for (int y = 0; y < actual.height(); ++y)
    for (int x = 0; x < actual.width(); ++x)
    {
      auto const a = actual.pixelColor(x, y), b = expected.pixelColor(x, y);
      TEST_LESS_OR_EQUAL(std::abs(a.red() - b.red()), 1, (x, y));
      TEST_LESS_OR_EQUAL(std::abs(a.green() - b.green()), 1, (x, y));
      TEST_LESS_OR_EQUAL(std::abs(a.blue() - b.blue()), 1, (x, y));
      shaded += a.red() < 250;
    }
  TEST_GREATER(shaded, 100u, ("The comparison must render visible shading"));
}
}  // namespace terrain_shade_tests
