#include "testing/testing.hpp"

#include "map/terrain_provider.hpp"

#include "storage/storage.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <array>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace terrain_provider_tests
{
namespace tests_support = platform::tests_support;

class ScopedDownloadQueue
{
public:
  ScopedDownloadQueue()
  {
    m_hadValue = settings::Get("DownloadQueue", m_saved);
    settings::Delete("DownloadQueue");
  }

  ~ScopedDownloadQueue()
  {
    if (m_hadValue)
      settings::Set("DownloadQueue", m_saved);
    else
      settings::Delete("DownloadQueue");
  }

private:
  std::string m_saved;
  bool m_hadValue = false;
};

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
  std::filesystem::create_directories(base::JoinPath(GetPlatform().WritableDir(), mapsDir));
  for (auto const & region : kFranceRegions)
  {
    tests_support::ScopedFile map(base::JoinPath(mapsDir, region + DATA_FILE_EXTENSION), "");
    std::filesystem::resize_file(map.GetFullPath(), storage.GetCountryFile(region).GetRemoteSize());
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
    return provider.RegisterBlock(file, invalidRect);
  }, [&](std::vector<terrain::TerrainId> const & ids)
  {
    m2::RectD invalidRect;
    provider.DeleteBlocks(ids, invalidRect);
  });
  provider.SetOnTerrainDeregisteredCallback([&](terrain::TwmFile const & file)
  { storage.OnTerrainFileDeregistered(file); });
}

UNIT_TEST(TerrainStorage_DeleteRegionKeepsSharedBlocks)
{
  WritableDirChanger const writableDirChanger("terrain_provider_delete_tests",
                                              WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;

  std::string grid;
  GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(grid);
  int64_t version = 0;
  std::vector<storage::Storage::TerrainBlock> blocks;
  std::map<storage::CountryId, std::vector<uint32_t>> coverage;
  storage::Storage::ParseTwmGridJson(grid, version, blocks, coverage);

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
    auto const & block = blocks[index];
    std::string const dir = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, std::to_string(block.m_version));
    std::filesystem::create_directories(dir);
    paths[index] = base::JoinPath(dir, block.m_id + TERRAIN_FILE_EXT);
    WriteTerrainHeader(paths[index], block.m_rect);
  }

  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  storage.OnTerrainScanned(storage::Storage::ScanTerrainFiles());

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
      expected.insert(blocks[index].m_id);
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
    storage.OnTerrainScanned(storage::Storage::ScanTerrainFiles());
    checkRemaining(i + 1);
  }
}

UNIT_TEST(TerrainStorage_DeleteKeepsOlderBlockCoveringAnotherRegion)
{
  WritableDirChanger const writableDirChanger("terrain_provider_old_grid_tests",
                                              WritableDirChanger::SettingsDirPolicy::UseWritableDir);
  ScopedDownloadQueue const guardSettings;
  storage::Storage storage;
  RegisterFranceMaps(storage);

  std::string const dir = base::JoinPath(GetPlatform().WritableDir(), TERRAIN_DIR, "1");
  std::filesystem::create_directories(dir);
  std::string const path = base::JoinPath(dir, "N43E005" TERRAIN_FILE_EXT);
  // The old file spans both current blocks shared by Marseille, Toulon and Nice.
  // Its name alone cannot identify its owners in the current grid.
  WriteTerrainHeader(path, terrain::GridBlock{2, 42, 8, 3}.GetRectMercator());
  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  storage.OnTerrainScanned(storage::Storage::ScanTerrainFiles());
  TEST_EQUAL(provider.GetRegisteredFiles().size(), 1, ());

  for (size_t i = 0; i < kFranceRegions.size(); ++i)
  {
    storage.DeleteNode(kFranceRegions[i]);
    bool const lastOwner = i + 1 == kFranceRegions.size();
    TEST_EQUAL(provider.GetRegisteredFiles().empty(), lastOwner, (kFranceRegions[i]));
    TEST_EQUAL(Platform::IsFileExistsByFullPath(path), !lastOwner, (kFranceRegions[i]));
    storage.OnTerrainScanned(storage::Storage::ScanTerrainFiles());
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
    TEST(provider.RegisterBlock(deletedFile, invalidRect), ());
    TEST(provider.RegisterBlock(keptFile, invalidRect), ());
    std::vector<terrain::TerrainId> deregistered;
    provider.SetOnTerrainDeregisteredCallback([&](terrain::TwmFile const & file)
    { deregistered.push_back(file.m_id); });

    invalidRect = {};
    provider.DeleteBlocks({deletedFile.m_id}, invalidRect);
    auto const registered = provider.GetRegisteredFiles();
    TEST_EQUAL(registered.size(), 1, (deletedPath, keptPath));
    TEST_EQUAL(registered.front().m_id, keptFile.m_id, ());
    TEST_EQUAL(deregistered, std::vector<terrain::TerrainId>{deletedFile.m_id}, ());
    TEST_EQUAL(invalidRect, deletedFile.m_rect, ());
    // The provider only deregisters; Storage owns physical file removal.
    TEST(Platform::IsFileExistsByFullPath(deletedPath), (deletedPath));
    TEST(Platform::IsFileExistsByFullPath(keptPath), (keptPath));
  }
}

}  // namespace terrain_provider_tests
