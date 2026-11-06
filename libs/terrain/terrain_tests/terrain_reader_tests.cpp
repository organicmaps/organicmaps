#include "testing/testing.hpp"

#include "terrain/terrain_reader.hpp"
#include "terrain/tile_mesh.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <map>

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
}  // namespace terrain_reader_tests
