#include "testing/testing.hpp"

#include "terrain/terrain_reader.hpp"
#include "terrain/tile_mesh.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <utility>
#include <vector>

namespace terrain_reader_tests
{
std::string GetTestBlockPath()
{
  return base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
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

// Pin physical triangles at every LOD without depending on vertex numbering or triangle order.
UNIT_TEST(TerrainReader_RealTinChecksums)
{
  std::array<std::pair<size_t, uint64_t>, 3> constexpr kExpected = {{
      {24167, 17649530687585639492ULL},
      {53571, 739089256235457921ULL},
      {2120, 14577312785597992943ULL},
  }};
  terrain::Reader const reader(GetTestBlockPath());
  auto const & header = reader.GetHeader();
  TEST_EQUAL(header.m_geometries.size(), kExpected.size(), ());
  for (size_t lod = 0; lod < kExpected.size(); ++lod)
  {
    terrain::TerrainData data;
    reader.ReadTerrain(header.GetLimitRect(), lod, data);
    auto const & mesh = data.m_mesh;
    auto const & indices = mesh.GetTriangles();
    using Vertex = std::pair<uint64_t, uint32_t>;  // Quantized position and altitude bits.
    using Triangle = std::array<Vertex, 3>;
    std::vector<Triangle> triangles;
    triangles.reserve(mesh.GetTrianglesCount());
    for (size_t i = 0; i < indices.size(); i += 3)
    {
      Triangle triangle;
      for (size_t j = 0; j < triangle.size(); ++j)
      {
        auto const v = indices[i + j];
        triangle[j] = {terrain::impl::PointKey(mesh.GetPointU(v)), static_cast<uint32_t>(mesh.GetAltitudes()[v])};
      }
      // Cyclic rotation preserves winding, which determines normals and contour direction.
      std::rotate(triangle.begin(), std::min_element(triangle.begin(), triangle.end()), triangle.end());
      triangles.push_back(triangle);
    }
    std::sort(triangles.begin(), triangles.end());
    uint64_t hash = 14695981039346656037ULL;
    for (auto const & triangle : triangles)
      for (auto const & [point, altitude] : triangle)
      {
        hash = (hash ^ point) * 1099511628211ULL;
        hash = (hash ^ altitude) * 1099511628211ULL;
      }
    TEST_EQUAL(triangles.size(), kExpected[lod].first, (lod));
    TEST_EQUAL(hash, kExpected[lod].second, (lod));
  }
}
}  // namespace terrain_reader_tests
