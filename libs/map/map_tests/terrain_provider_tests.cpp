#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/terrain_provider.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"

#include "storage/storage.hpp"

#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"
#include "platform/settings.hpp"

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

class ScopedDownloadQueue
{
public:
  ScopedDownloadQueue()
  {
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue"})
    {
      std::string value;
      if (settings::Get(key, value))
        m_saved.emplace(key, std::move(value));
      settings::Delete(key);
    }
  }

  ~ScopedDownloadQueue()
  {
    for (auto const * key : {"DownloadQueue", "TerrainDownloadQueue"})
      if (auto const it = m_saved.find(key); it != m_saved.end())
        settings::Set(key, it->second);
      else
        settings::Delete(key);
  }

private:
  std::map<std::string, std::string> m_saved;
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
    return provider.RegisterBlock(file, &invalidRect);
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
    TEST(Platform::MkDirRecursively(dir), ());
    paths[index] = base::JoinPath(dir, block.m_id + TERRAIN_FILE_EXT);
    WriteTerrainHeader(paths[index], block.m_rect);
  }

  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  storage.OnTerrainScanned(storage.ScanTerrainFiles());

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
    storage.OnTerrainScanned(storage.ScanTerrainFiles());
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
  TEST(Platform::MkDirRecursively(dir), ());
  std::string const path = base::JoinPath(dir, "N43E005" TERRAIN_FILE_EXT);
  // The old file spans both current blocks shared by Marseille, Toulon and Nice.
  // Its name alone cannot identify its owners in the current grid.
  WriteTerrainHeader(path, terrain::GridBlock{2, 42, 8, 3}.GetRectMercator());
  terrain::TerrainProvider provider;
  ConnectTerrainCallbacks(storage, provider);
  storage.OnTerrainScanned(storage.ScanTerrainFiles());
  TEST_EQUAL(provider.GetRegisteredFiles().size(), 1, ());

  for (size_t i = 0; i < kFranceRegions.size(); ++i)
  {
    storage.DeleteNode(kFranceRegions[i]);
    bool const lastOwner = i + 1 == kFranceRegions.size();
    TEST_EQUAL(provider.GetRegisteredFiles().empty(), lastOwner, (kFranceRegions[i]));
    TEST_EQUAL(Platform::IsFileExistsByFullPath(path), !lastOwner, (kFranceRegions[i]));
    storage.OnTerrainScanned(storage.ScanTerrainFiles());
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
    auto affected = deletedFile.m_rect;
    affected.Add(keptFile.m_rect);
    TEST_EQUAL(invalidRect, affected, ("The kept neighbor must rebuild halo normals and contour guide points"));
    // The provider only deregisters; Storage owns physical file removal.
    TEST(Platform::IsFileExistsByFullPath(deletedPath), (deletedPath));
    TEST(Platform::IsFileExistsByFullPath(keptPath), (keptPath));
  }
}

namespace
{
struct ElevationSample
{
  double m_lat, m_lon, m_altitude;
  bool m_raster;
  double m_maxError = 5.01;
};

std::array<ElevationSample, 4> const kElevationSamples = {{
    {44.0, 7.5, 756.0, false},
    {43.0 + 275.0 / 360.0, 7.0 + 80.0 / 360.0, 318.0, true, 3.01},
    {43.25, 7.5, -2342.0, true, 3.01},
    {43.0 + 257.0 / 360.0, 7.0 + 125.0 / 360.0, 0.0, true, 1e-6},
}};

m2::PointD GetSamplePoint(ElevationSample const & sample)
{
  // Use the stored lattice node, avoiding projection rounding at the file boundary.
  return PointUToPointD(PointDToPointU(mercator::FromLatLon(sample.m_lat, sample.m_lon), terrain::kTerrainCoordBits),
                        terrain::kTerrainCoordBits);
}

terrain::TwmFile GetElevationTestFile()
{
  terrain::TwmFile file;
  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), "N43E007.twm");
  TEST_EQUAL(terrain::TwmSet::ReadFile(path, 1, file), terrain::TwmSet::RegResult::Success, (path));
  return file;
}

class ElevationTestFramework : public Framework
{
public:
  ElevationTestFramework() : Framework({}, false /* loadMaps */)
  {
    // This test registers a shared resource, outside Storage ownership.
    m_terrainProvider.SetOnTerrainDeregisteredCallback({});
  }

  void RegisterTerrain(terrain::TwmFile const & file) { TEST(m_terrainProvider.RegisterBlock(file), ()); }
};
}  // namespace

UNIT_TEST(TerrainReader_RealHybridElevation)
{
  terrain::Reader reader(GetElevationTestFile().m_path);
  auto const & header = reader.GetHeader();
  TEST_GREATER(header.m_geometries.size(), 1, ());
  auto const finest = header.m_geometries.size() - 1;
  TEST(header.HasRaster(finest), ());

  terrain::TerrainData coarse(header.m_coordBits), fine(header.m_coordBits);
  reader.ReadTerrain(header.GetLimitRect(), 0, coarse);
  TEST(!coarse.m_mesh.IsEmpty(), ());
  TEST(coarse.m_rasterPatches.empty(), ());
  reader.ReadTerrain(header.GetLimitRect(), finest, fine);
  TEST(!fine.m_mesh.IsEmpty(), ("The finest LOD retains the TIN border"));
  TEST(!fine.m_rasterPatches.empty(), ());
  for (auto const & sample : kElevationSamples)
  {
    auto const point = GetSamplePoint(sample);
    bool raster = false;
    for (auto const & patch : fine.m_rasterPatches)
      raster = raster || patch.GetRect().IsPointInside(point);
    TEST_EQUAL(raster, sample.m_raster, (point));
    auto const elevation = reader.GetElevation(point);
    TEST(elevation, (point));
    TEST_ALMOST_EQUAL_ABS(*elevation, sample.m_altitude, sample.m_maxError, (point));
  }
  auto const coarseElevation = coarse.GetElevation(GetSamplePoint(kElevationSamples[1]));
  TEST(coarseElevation, ());
  TEST_GREATER(std::abs(*coarseElevation - kElevationSamples[1].m_altitude), 5.01,
               ("This sample distinguishes the finest surface from the coarsest TIN"));
  // Source-grid corners: SW 243, SE 491, NE 259, NW 386 m. The rendered
  // SW-NE triangles give 371 m here; bilinear interpolation gives 394.4375 m.
  auto const sw = GetSamplePoint({43.0 + 319.0 / 360.0, 7.0 + 89.0 / 360.0, 0.0, true});
  auto const ne = GetSamplePoint({43.0 + 320.0 / 360.0, 7.0 + 90.0 / 360.0, 0.0, true});
  auto const interpolated = reader.GetElevation({sw.x + 0.75 * (ne.x - sw.x), sw.y + 0.25 * (ne.y - sw.y)});
  TEST(interpolated, ());
  TEST_ALMOST_EQUAL_ABS(*interpolated, 371.0, 3.01, ());
  TEST(!reader.GetElevation({0, 0}), ());
}

UNIT_TEST(TerrainProvider_RealHybridElevation)
{
  terrain::TerrainProvider provider;
  auto const file = GetElevationTestFile();
  auto const point = GetSamplePoint(kElevationSamples[1]);
  TEST(!provider.GetElevation(point), ());
  TEST(provider.RegisterBlock(file), ());
  for (auto const & sample : kElevationSamples)
  {
    auto const p = GetSamplePoint(sample);
    for (double const wrap : {0.0, mercator::Bounds::kRangeX, -mercator::Bounds::kRangeX})
    {
      auto const elevation = provider.GetElevation({p.x + wrap, p.y});
      TEST(elevation, (p, wrap));
      TEST_ALMOST_EQUAL_ABS(*elevation, sample.m_altitude, sample.m_maxError, (p, wrap));
    }
  }
  TEST(!provider.GetElevation({0, 0}), ());
  m2::RectD invalidRect;
  provider.DeleteBlocks({file.m_id}, invalidRect);
  TEST(!provider.GetElevation(point), ());
  TEST(provider.GetRegisteredFiles().empty(), ());
}

using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, Framework_RealTerrainPlacePage)
{
  ElevationTestFramework framework;
  place_page::BuildInfo build;
  build.m_match = place_page::BuildInfo::Match::Nothing;
  build.m_mercator = GetSamplePoint(kElevationSamples[1]);
  framework.BuildAndSetPlacePageInfo(build);
  TEST(!framework.GetCurrentPlacePageInfo().GetTerrainElevation(), ());
  TEST(framework.GetCurrentPlacePageInfo().GetFormattedTerrainElevation().empty(), ());

  framework.RegisterTerrain(GetElevationTestFile());
  framework.BuildAndSetPlacePageInfo(build);
  auto const & info = framework.GetCurrentPlacePageInfo();
  TEST(!info.IsFeature(), ());
  TEST(info.GetTerrainElevation(), ());
  TEST_ALMOST_EQUAL_ABS(*info.GetTerrainElevation(), kElevationSamples[1].m_altitude, kElevationSamples[1].m_maxError,
                        ());
  TEST(!info.GetFormattedTerrainElevation().empty(), ());
  TEST(info.GetSubtitle().find("▲ " + info.GetFormattedTerrainElevation()) != std::string::npos, (info.GetSubtitle()));

  build.m_mercator = {0, 0};
  framework.BuildAndSetPlacePageInfo(build);
  TEST(!framework.GetCurrentPlacePageInfo().GetTerrainElevation(), ());
  TEST(framework.GetCurrentPlacePageInfo().GetFormattedTerrainElevation().empty(), ());
  TEST(framework.GetCurrentPlacePageInfo().GetSubtitle().find("▲") == std::string::npos, ());
}

}  // namespace terrain_provider_tests
