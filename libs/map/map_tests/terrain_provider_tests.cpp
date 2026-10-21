#include "testing/testing.hpp"

#include "map/terrain_provider.hpp"

#include "indexer/terrain/terrain_reader.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"

namespace terrain_provider_tests
{
// Every geometry scale of a registered block serves the tile reads.
UNIT_TEST(TerrainProvider_RegisterAndRead)
{
  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
  terrain::TwmFile file;
  TEST_EQUAL(terrain::TwmSet::ReadFile(path, 1, file), terrain::TwmSet::RegResult::Success, (path));

  terrain::TerrainProvider provider;
  TEST(!provider.HasTerrain(file.m_rect), ());
  TEST(provider.RegisterBlock(file), ());
  TEST(provider.RegisterBlock(file), ("Already registered"));
  TEST(provider.HasTerrain(file.m_rect), ());
  TEST(!provider.HasTerrain({0, 0, 1, 1}), ());
  TEST_EQUAL(provider.GetRegisteredFiles().size(), 1, ());

  auto const point = mercator::FromLatLon(43.0 + 275.0 / 360.0, 7.0 + 80.0 / 360.0);
  m2::RectD rect(point, point);
  rect.Inflate(0.01, 0.01);
  for (auto const & geometry : terrain::Reader(path).GetHeader().m_geometries)
  {
    terrain::TerrainData data;
    provider.ReadTerrain(rect, geometry.m_scale, data);
    TEST(!data.IsEmpty(), (geometry.m_scale));
  }

  provider.Clear();
  TEST(!provider.HasTerrain(file.m_rect), ());
}
}  // namespace terrain_provider_tests
