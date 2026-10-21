#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/terrain_shade_shape.hpp"

#include <cmath>
#include <vector>

namespace terrain_shade_gpu_test
{
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
}  // namespace terrain_shade_gpu_test
