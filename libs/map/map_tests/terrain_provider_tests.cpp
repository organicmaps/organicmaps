#include "testing/testing.hpp"

#include "map/terrain_provider.hpp"

#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/tile_key.hpp"

#include "storage/storage.hpp"

#include "indexer/scales.hpp"
#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/terrain_serdes.hpp"
#include "indexer/terrain/terrain_utils.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <array>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace terrain_provider_tests
{
namespace tests_support = platform::tests_support;

void WriteTerrainHeader(std::string const & path, m2::RectD const & rect)
{
  terrain::TwmHeader header;
  header.m_geometries = {{17, 0}};
  header.m_limitLB = PointDToPointU(rect.LeftBottom(), header.m_coordBits);
  header.m_limitRT = PointDToPointU(rect.RightTop(), header.m_coordBits);
  // Registration and deletion need only the header, without any mesh sections.
  FilesContainerW container(path);
  auto writer = container.GetWriter(terrain::kHeaderTag);
  header.Serialize(*writer);
}

std::array<storage::CountryId, 3> const kFranceRegions = {"France_Provence-Alpes-Cote dAzur_Bouches-du-Rhone",
                                                          "France_Provence-Alpes-Cote dAzur_Var",
                                                          "France_Provence-Alpes-Cote dAzur_Maritime Alps"};

void RegisterFranceMaps(storage::Storage & storage)
{
  storage.Init({}, [](auto const &, auto const &) { return false; });
  std::string const mapsDir = std::to_string(storage.GetCurrentDataVersion());
  TEST(Platform::MkDirRecursively(base::JoinPath(GetPlatform().WritableDir(), mapsDir)), ());
  for (auto const & region : kFranceRegions)
  {
    tests_support::ScopedFile map(base::JoinPath(mapsDir, region + DATA_FILE_EXTENSION), "");
    FileWriter writer(map.GetFullPath());
    writer.Seek(storage.GetCountryFile(region).GetRemoteSize() - 1);
    writer.Write("", 1);
    map.Reset();  // DeleteNode removes it; the writable directory guard handles failures.
  }
  storage.RegisterAllLocalMaps();
}

void ConnectTerrainCallbacks(storage::Storage & storage, terrain::TerrainProvider & provider)
{
  storage.SetTerrainCallbacks(
      [&](terrain::TwmFile const & file)
  {
    m2::RectD invalidRect;
    TEST(provider.RegisterBlock(file, &invalidRect), (file.m_path));
  }, [&](std::vector<terrain::TwmFile> const & files)
  {
    std::vector<terrain::TerrainId> ids;
    for (auto const & file : files)
      ids.push_back(file.m_id);
    m2::RectD invalidRect;
    provider.DeleteBlocks(ids, invalidRect);
  });
  provider.SetOnTerrainDeregisteredCallback([&](terrain::TwmFile const & file)
  { storage.OnTerrainFileDeregistered(file); });
}

// Mirrors Framework::RegisterAllMaps.
void ScanTerrain(storage::Storage & storage, terrain::TerrainProvider & provider)
{
  storage.RegisterAllLocalTerrain();
  for (auto const & file : storage.GetLocalTerrainFiles())
    TEST(provider.RegisterBlock(file), (file.m_path));
}

UNIT_TEST(TerrainStorage_DeleteRegionKeepsSharedBlocks)
{
  WritableDirChanger const writableDirChanger("terrain_provider_delete_tests",
                                              WritableDirChanger::SettingsDirPolicy::UseWritableDir);

  std::string content;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
  storage::TerrainGrid grid;
  grid.Parse(content);
  auto const & coverage = grid.GetCoverage();

  // Marseille and Toulon share two terrain blocks, with Nice covered by one of them.
  storage::Storage storage;
  RegisterFranceMaps(storage);
  std::set<uint32_t> created;
  for (auto const & region : kFranceRegions)
  {
    auto const & indices = coverage.at(region);
    created.insert(indices.begin(), indices.end());
  }

  std::map<uint32_t, std::string> paths;
  for (auto const index : created)
  {
    auto const & block = grid.GetBlock(index);
    std::string const dir = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, std::to_string(block.m_version));
    TEST(Platform::MkDirRecursively(dir), ());
    paths[index] = base::JoinPath(dir, block.m_id + TERRAIN_FILE_EXT);
    WriteTerrainHeader(paths[index], block.m_rect);
  }

  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  ScanTerrain(storage, provider);

  auto const checkRemaining = [&](size_t firstRegion)
  {
    std::set<uint32_t> wanted;
    for (size_t i = firstRegion; i < kFranceRegions.size(); ++i)
    {
      auto const & indices = coverage.at(kFranceRegions[i]);
      wanted.insert(indices.begin(), indices.end());
      TEST(storage.GetLatestLocalFile(kFranceRegions[i]) != nullptr, (kFranceRegions[i]));
      TEST_EQUAL(storage.GetTerrainAttrs(kFranceRegions[i]).m_status, storage::Storage::TerrainStatus::OnDisk,
                 (kFranceRegions[i]));
    }

    std::set<terrain::TerrainId> expected, registered;
    for (auto const index : wanted)
      expected.insert(grid.GetBlock(index).m_id);
    for (auto const & file : provider.GetRegisteredFiles())
      registered.insert(file.m_id);
    TEST_EQUAL(registered, expected, (firstRegion));
    for (auto const & [index, path] : paths)
      TEST_EQUAL(Platform::IsFileExistsByFullPath(path), wanted.count(index) != 0, (path));
  };

  checkRemaining(0);
  for (size_t i = 0; i < kFranceRegions.size(); ++i)
  {
    storage.DeleteNode(kFranceRegions[i]);
    TEST(storage.GetLatestLocalFile(kFranceRegions[i]) == nullptr, (kFranceRegions[i]));
    checkRemaining(i + 1);
    // A new scan must agree with the incremental status after each deletion.
    ScanTerrain(storage, provider);
    checkRemaining(i + 1);
  }
}

UNIT_TEST(TerrainStorage_DeleteKeepsOlderBlockCoveringAnotherRegion)
{
  WritableDirChanger const writableDirChanger("terrain_provider_old_grid_tests",
                                              WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  storage::Storage storage;
  RegisterFranceMaps(storage);

  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1");
  TEST(Platform::MkDirRecursively(dir), ());
  std::string const path = base::JoinPath(dir, "N43E005" TERRAIN_FILE_EXT);
  // The old file spans both current blocks shared by Marseille, Toulon and Nice.
  // Its name alone cannot identify its owners in the current grid.
  WriteTerrainHeader(path, terrain::GridBlock{2, 42, 8, 3}.GetRectMercator());
  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  ScanTerrain(storage, provider);
  TEST_EQUAL(provider.GetRegisteredFiles().size(), 1, ());

  for (size_t i = 0; i < kFranceRegions.size(); ++i)
  {
    storage.DeleteNode(kFranceRegions[i]);
    bool const lastOwner = i + 1 == kFranceRegions.size();
    TEST_EQUAL(provider.GetRegisteredFiles().empty(), lastOwner, (kFranceRegions[i]));
    TEST_EQUAL(Platform::IsFileExistsByFullPath(path), !lastOwner, (kFranceRegions[i]));
    ScanTerrain(storage, provider);
    TEST_EQUAL(provider.GetRegisteredFiles().empty(), lastOwner, (kFranceRegions[i]));
  }
}

UNIT_TEST(TerrainProvider_DeleteByIdKeepsAdjacentBlocks)
{
  std::array const cases = {std::array{terrain::GridBlock{2, 42, 3, 3}, terrain::GridBlock{5, 43, 5, 2}},
                            std::array{terrain::GridBlock{5, 43, 5, 2}, terrain::GridBlock{5, 35, 5, 8}}};
  for (auto const & [deleted, kept] : cases)
  {
    std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "terrain_provider_adjacent_tests");
    tests_support::ScopedDirCleanup const cleanup(dir);
    std::string const deletedPath = base::JoinPath(dir, deleted.GetFileName());
    std::string const keptPath = base::JoinPath(dir, kept.GetFileName());
    WriteTerrainHeader(deletedPath, deleted.GetRectMercator());
    WriteTerrainHeader(keptPath, kept.GetRectMercator());
    terrain::TwmFile deletedFile, keptFile;
    TEST_EQUAL(terrain::TwmSet::ReadFile(deletedPath, 1, deletedFile), terrain::TwmSet::RegResult::Success, ());
    TEST_EQUAL(terrain::TwmSet::ReadFile(keptPath, 1, keptFile), terrain::TwmSet::RegResult::Success, ());
    terrain::TerrainProvider provider;
    m2::RectD invalidRect;
    TEST(provider.RegisterBlock(deletedFile, &invalidRect), ());
    TEST(provider.RegisterBlock(keptFile, &invalidRect), ());
    std::vector<terrain::TerrainId> deregistered;
    provider.SetOnTerrainDeregisteredCallback([&](terrain::TwmFile const & file)
    { deregistered.push_back(file.m_id); });

    invalidRect = {};
    provider.DeleteBlocks({deletedFile.m_id}, invalidRect);
    auto const registered = provider.GetRegisteredFiles();
    TEST_EQUAL(registered.size(), 1, (deletedPath, keptPath));
    TEST_EQUAL(registered.front().m_id, keptFile.m_id, ());
    TEST_EQUAL(deregistered, std::vector<terrain::TerrainId>{deletedFile.m_id}, ());
    // Only the neighbor tiles along the shared edge rebuild their halo normals and contours.
    TEST(invalidRect.IsRectInside(deletedFile.m_rect), (invalidRect));
    auto edge = invalidRect;
    TEST(edge.Intersect(keptFile.m_rect) && !edge.IsEmptyInterior(), (invalidRect));
    TEST(!invalidRect.IsRectInside(keptFile.m_rect), (invalidRect));
    // The provider only deregisters; Storage owns physical file removal.
    TEST(Platform::IsFileExistsByFullPath(deletedPath), (deletedPath));
    TEST(Platform::IsFileExistsByFullPath(keptPath), (keptPath));
  }
}

UNIT_TEST(TerrainProvider_InvalidationCoversTheTileReadOverhang)
{
  // A coarsest terrain tile beside a block reads it for the contours smoothing and the shade normals.
  df::TileKey const key(13, 0, terrain::kMinIsolinesZoom);
  terrain::GridBlock const block{5, 0, 5, 5};
  auto const tileRect = key.GetGlobalRect();
  TEST(!tileRect.IsIntersect(block.GetRectMercator()), (tileRect));
  TEST(df::GetTerrainReadRect(tileRect).IsIntersect(block.GetRectMercator()), (tileRect));

  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), "terrain_provider_overhang_tests");
  tests_support::ScopedDirCleanup const cleanup(dir);
  std::string const path = base::JoinPath(dir, block.GetFileName());
  WriteTerrainHeader(path, block.GetRectMercator());
  terrain::TwmFile file;
  TEST_EQUAL(terrain::TwmSet::ReadFile(path, 1, file), terrain::TwmSet::RegResult::Success, ());
  terrain::TerrainProvider provider;
  m2::RectD invalidRect;
  TEST(provider.RegisterBlock(file, &invalidRect), ());
  TEST(invalidRect.IsIntersect(tileRect), (invalidRect));
  invalidRect = {};
  provider.DeleteBlocks({file.m_id}, invalidRect);
  TEST(invalidRect.IsIntersect(tileRect), (invalidRect));
}

// The raster reads extend the invalidation by their halo, the cached neighbor tiles read within it.
UNIT_TEST(TerrainProvider_RasterReadsExtendInvalidation)
{
  terrain::TwmFile file;
  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
  TEST_EQUAL(terrain::TwmSet::ReadFile(path, 1, file), terrain::TwmSet::RegResult::Success, (path));
  terrain::TerrainProvider provider;
  TEST(provider.RegisterBlock(file), ());

  auto const point = mercator::FromLatLon(43.0 + 275.0 / 360.0, 7.0 + 80.0 / 360.0);
  terrain::TerrainData data;
  provider.ReadTerrain({point, point}, scales::GetUpperScale(), data);
  TEST(!data.m_rasterPatches.empty(), ());
  m2::RectD invalidRect;
  provider.DeleteBlocks({file.m_id}, invalidRect);
  TEST(invalidRect.IsRectInside(file.m_rect), (invalidRect));
  TEST_GREATER(invalidRect.SizeX(), file.m_rect.SizeX() + 1e-6, (invalidRect, file.m_rect));
  TEST(provider.GetRegisteredFiles().empty(), ());
}

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
