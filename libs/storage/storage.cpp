#include "storage/storage.hpp"

#include "storage/country_tree_helpers.hpp"
#include "storage/diff_scheme/apply_diff.hpp"
// #include "storage/diff_scheme/diff_scheme_loader.hpp"
#include "storage/downloader.hpp"
#include "storage/map_files_downloader.hpp"
#include "storage/storage_helpers.hpp"

#include "indexer/terrain/twm_set.hpp"

#include "platform/downloader_utils.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/blake3.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <array>
#include <sstream>
#include <string_view>
#include <utility>

namespace storage
{
using namespace downloader;
using namespace generator::mwm_diff;
using namespace platform;
namespace
{
std::string const kDownloadQueueKey = "DownloadQueue";
std::string const kTerrainDownloadQueueKey = "TerrainDownloadQueue";

bool IsRealObsolete(LocalFilePtr const & lf);

void SaveQueueIfChanged(std::string const & key, std::string const & value)
{
  std::string previous;
  if (!settings::Get(key, previous) || previous != value)
    settings::Set(key, value);
}

// Editing maps older than approximately three months old is disabled, since the data
// is most likely already fixed on OSM. Not limited to the latest one or two versions,
// because a user can forget to update maps after a new app version has been installed
// automatically in the background.
uint64_t const kMaxSecondsTillLastVersionUpdate = 3600 * 24 * 31 * 3;
// Editing maps older than approximately six months old is disabled, because the device
// may have been offline for a long time.
uint64_t const kMaxSecondsTillNoEdits = 3600 * 24 * 31 * 6;

void DeleteCountryIndexes(LocalCountryFile const & localFile)
{
  platform::CountryIndexes::DeleteFromDisk(localFile);
}

void DeleteFromDiskWithIndexes(LocalCountryFile const & localFile, MapFileType type)
{
  DeleteCountryIndexes(localFile);
  localFile.DeleteFromDisk(type);
}

CountryTree::Node const & LeafNodeFromCountryId(CountryTree const & root, CountryId const & countryId)
{
  CountryTree::Node const * node = root.FindFirstLeaf(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree as a leaf."));
  return *node;
}

bool HasDownloadedFile(std::string const & readyFilePath, MapFileType type)
{
  // Downloaded diffs can have either .diff or .diff.ready extension.
  bool isDownloadedDiff = false;
  if (type == MapFileType::Diff)
  {
    std::string filePath = readyFilePath;
    base::GetNameWithoutExt(filePath);
    isDownloadedDiff = GetPlatform().IsFileExistsByFullPath(filePath);
  }

  // Existing files skip the transfer but still follow the validation and registration path.
  return isDownloadedDiff || GetPlatform().IsFileExistsByFullPath(readyFilePath);
}
}  // namespace

CountriesSet GetQueuedCountries(QueueInterface const & queue)
{
  CountriesSet result;
  queue.ForEachCountry([&result](QueuedCountry const & country) { result.insert(country.GetCountryId()); });

  return result;
}

downloader::Progress Storage::GetOverallProgress(CountriesVec const & countries) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  downloader::Progress overallProgress;
  auto const mwmsInQueue = GetQueuedCountries(m_downloader->GetQueue());
  for (auto const & country : CountriesSet(countries.begin(), countries.end()))
  {
    ASSERT(IsLeaf(country), (country));
    auto const progress = GetMapProgress(country, mwmsInQueue, true /* includeInstalled */);
    overallProgress.m_bytesDownloaded += progress.m_bytesDownloaded;
    overallProgress.m_bytesTotal += progress.m_bytesTotal;
  }
  return overallProgress;
}

Progress Storage::GetDownloadingProgress(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_countries.FindFirst(countryId), (countryId));
  Progress progress;
  auto maps = GetActiveMapProgress(countryId);
  auto terrain = GetTerrainDownloadingProgress(countryId, maps.m_bytesTotal > 0 /* includeCompleted */);
  if (terrain.m_bytesTotal > 0)
    maps = GetActiveMapProgress(countryId, true /* includeCompleted */);
  progress.m_mwmDownloaded = maps.m_bytesDownloaded;
  progress.m_mwmTotal = maps.m_bytesTotal;
  progress.m_twmDownloaded = terrain.m_bytesDownloaded;
  progress.m_twmTotal = terrain.m_bytesTotal;
  return progress;
}

Storage::Storage(int)
{
  // Do nothing here, used in RunCountriesCheckAsync() only.
}

Storage::Storage(std::string const & pathToCountriesFile /* = COUNTRIES_FILE */,
                 std::string const & dataDir /* = string() */)
  : m_downloader(GetDownloader())
  , m_dataDir(dataDir)
{
  m_downloader->SetDownloadingPolicy(m_downloadingPolicy);

  SetLocale(languages::GetCurrentTwine());
  LoadCountriesFile(pathToCountriesFile);

  // The terrain blocks grid ships next to the countries file; the terrain features
  // simply stay off without it (e.g. minimal test bundles).
  try
  {
    std::string content;
    GetPlatform().GetReader(TERRAIN_GRID_FILE)->ReadAsString(content);
    m_terrainGrid.Parse(content);
    // The grid and the countries file are generated together.
    ASSERT(base::AllOf(m_terrainGrid.GetCoverage(), [this](auto const & entry) { return IsLeaf(entry.first); }), ());
    m_downloader->SetTerrainDataVersion(m_terrainGrid.GetVersion());
    LOG(LINFO, ("Terrain grid:", m_terrainGrid.GetBlocksCount(), "blocks,", m_terrainGrid.GetCoverage().size(),
                "regions, version", m_terrainGrid.GetVersion()));
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Terrain grid is not available:", ex.Msg()));
  }

  m_downloader->SetDataVersion(m_currentVersion);

  std::string debugServer;
  if (GetDebugMapDownloadServer(debugServer))
    m_downloader->SetServersList({debugServer});
}

Storage::Storage(std::string const & referenceCountriesTxtJsonForTesting,
                 std::unique_ptr<MapFilesDownloader> mapDownloaderForTesting)
  : m_downloader(std::move(mapDownloaderForTesting))
{
  m_downloader->SetDownloadingPolicy(m_downloadingPolicy);

  m_currentVersion = LoadCountriesFromBuffer(referenceCountriesTxtJsonForTesting, m_countries, m_countriesInfo);
  CHECK_LESS_OR_EQUAL(0, m_currentVersion, ("Can't load test countries file"));

  m_downloader->SetDataVersion(m_currentVersion);
}

namespace twm_grid_json
{
// The grid uses countries.json-style "s" size and truncated-BLAKE3 "h" hash per block,
// the extents in whole degrees, the per-block
// version "v" (the top-level index version when omitted) and the region coverage.
struct Block
{
  std::string id;
  int sx = 0;
  int sy = 0;
  uint64_t s = 0;
  std::string h;
  int64_t v = 0;
};
struct Grid
{
  int64_t v = -1;
  std::vector<Block> blocks;
  std::map<std::string, std::vector<std::string>> mwms;
};
}  // namespace twm_grid_json

void TerrainGrid::Parse(std::string const & jsonBuffer)
{
  twm_grid_json::Grid grid;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(grid, jsonBuffer); error)
    MYTHROW(RootException, ("Can't parse the terrain grid:", glz::format_error(error, jsonBuffer)));

  if (grid.v <= 0)
    MYTHROW(RootException, ("Invalid terrain grid version", grid.v));

  // Into the locals first: a throw below must not leave the grid half-configured
  // (a truncated coverage would silently download and render terrain with holes).
  std::vector<Block> blocks;
  blocks.reserve(grid.blocks.size());
  std::map<terrain::TerrainId, uint32_t> indices;
  for (auto const & json : grid.blocks)
  {
    terrain::GridBlock gridBlock;
    gridBlock.m_width = json.sx;
    gridBlock.m_height = json.sy;
    if (!terrain::ParseBlockName(json.id, gridBlock.m_bottom, gridBlock.m_left) || !terrain::IsValidBlock(gridBlock) ||
        json.s == 0 || json.h.empty() || json.v < 0)
    {
      MYTHROW(RootException, ("Invalid terrain grid block", json.id));
    }
    if (!indices.emplace(json.id, static_cast<uint32_t>(blocks.size())).second)
      MYTHROW(RootException, ("Duplicate terrain grid block", json.id));
    blocks.push_back({json.id, gridBlock.GetRectMercator(), json.s, json.h, json.v > 0 ? json.v : grid.v});
  }

  // The coverage ships with the grid: a dangling block id means a broken bundle.
  if (grid.mwms.empty())
    MYTHROW(RootException, ("No region coverage in the terrain grid"));
  std::map<CountryId, Coverage> coverage;
  std::vector<CountriesSet> regions(blocks.size());
  for (auto const & [region, ids] : grid.mwms)
  {
    if (ids.empty())
      MYTHROW(RootException, ("Empty coverage of", region));
    auto & regionBlocks = coverage[region];
    regionBlocks.reserve(ids.size());
    for (auto const & id : ids)
    {
      auto const it = indices.find(id);
      if (it == indices.end())
        MYTHROW(RootException, ("Unknown block", id, "in the coverage of", region));
      regionBlocks.push_back(it->second);
      regions[it->second].insert(region);
    }
    // Sorted like the group unions; a duplicate id must not double-count the sizes.
    base::SortUnique(regionBlocks);
  }

  m_version = grid.v;
  m_blocks = std::move(blocks);
  m_indices = std::move(indices);
  m_coverage = std::move(coverage);
  m_regions = std::move(regions);
}

std::optional<uint32_t> TerrainGrid::FindIndex(terrain::TerrainId const & id) const
{
  auto const it = m_indices.find(id);
  if (it == m_indices.end())
    return {};
  return it->second;
}

TerrainGrid::Coverage const * TerrainGrid::FindCoverage(CountryId const & region) const
{
  auto const it = m_coverage.find(region);
  return it == m_coverage.end() ? nullptr : &it->second;
}

void Storage::Init(UpdateCallback didDownload, DeleteCallback willDelete)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_didDownload = std::move(didDownload);
  m_willDelete = std::move(willDelete);
}

void Storage::SetTerrainCallbacks(TerrainRegisterFn registerFn, TerrainDeleteFn deleteFn)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(registerFn && deleteFn, ());
  m_terrainRegisterFn = std::move(registerFn);
  m_terrainDeleteFn = std::move(deleteFn);
}

void Storage::DeleteTerrain(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CancelTerrain(countryId);

  CountriesSet subtree;
  ForEachInSubtree(countryId, [&subtree](CountryId const & id, bool /* groupNode */) { subtree.insert(id); });
  DeleteUnwantedTerrain(countryId, subtree);
}

void Storage::DeleteUnwantedTerrain(CountryId const & countryId, CountriesSet const & excluded)
{
  auto const covering = GetCoveringBlocks(countryId);
  auto const wanted = GetWantedTerrainBlocks(excluded);

  std::vector<terrain::TerrainId> ids;
  CountriesSet affected{countryId};
  for (auto const & [id, local] : m_localTerrainFiles)
  {
    auto const & file = local.m_file;
    auto const index = m_terrainGrid.FindIndex(id);
    bool remove = false;
    if (index && m_terrainGrid.GetBlock(*index).m_version == file.m_version)
      remove = std::binary_search(covering.begin(), covering.end(), *index) && !wanted.contains(*index);
    else
    {
      // Only older layouts need geometry: their IDs can cover several current cells.
      auto const covers = [&](uint32_t i)
      { return terrain::IsInteriorOverlap(file.m_rect, m_terrainGrid.GetBlock(i).m_rect); };
      remove = base::AnyOf(covering, covers) && !base::AnyOf(wanted, covers);
    }
    if (!remove)
      continue;
    ids.push_back(id);
    if (index)
    {
      auto const & regions = m_terrainGrid.GetRegions(*index);
      affected.insert(regions.begin(), regions.end());
    }
  }
  if (ids.empty())
    return;
  DeleteTerrainFiles(ids);
  NotifyStatusChangedForHierarchy(affected);
}

void Storage::DeleteTerrainFiles(std::vector<terrain::TerrainId> const & ids)
{
  if (ids.empty())
    return;

  std::vector<terrain::TwmFile> files;
  for (auto const & id : ids)
  {
    auto const it = m_localTerrainFiles.find(id);
    ASSERT(it != m_localTerrainFiles.end(), (id));
    files.push_back(it->second.m_file);
    m_localTerrainFiles.erase(it);
  }
  m_terrainDeleteFn(files);
}

void Storage::OnTerrainFileDeregistered(terrain::TwmFile const & file)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  // The provider also deregisters a file it fails to open (see ValueSetBase::LockValue);
  // a completion for an old version must leave its replacement registered.
  if (auto const it = m_localTerrainFiles.find(file.m_id);
      it != m_localTerrainFiles.end() && it->second.m_file.m_path == file.m_path)
    m_localTerrainFiles.erase(it);
  // Keep directories prepared for queued downloads. Startup scanning removes empty ones.
  Platform::RemoveFileIfExists(file.m_path);
}

template <class Fn>
void Storage::ForEachCoverageLeaf(CountryId const & countryId, Fn && fn) const
{
  if (m_terrainGrid.IsEmpty())  // No terrain grid in the bundle: skip the tree walks.
    return;

  if (auto const * coverage = m_terrainGrid.FindCoverage(countryId))
  {
    fn(countryId, *coverage);
    return;
  }

  // A group node: every coverage leaf of its subtree.
  ForEachInSubtree(countryId, [&](CountryId const & id, bool groupNode)
  {
    if (groupNode)
      return;
    if (auto const * coverage = m_terrainGrid.FindCoverage(id))
      fn(id, *coverage);
  });
}

LocalFilePtr Storage::GetInstalledLocalFile(CountryId const & countryId) const
{
  auto const local = GetLatestLocalFile(countryId);
  if (local && local->OnDisk(MapFileType::Map))
    return local;
  if (auto const it = m_countriesInfo.m_mwmToOld.find(countryId); it != m_countriesInfo.m_mwmToOld.end())
  {
    auto const old = GetLatestLocalFile(it->second);
    if (old && old->OnDisk(MapFileType::Map) && IsRealObsolete(old))
      return old;
  }
  return {};
}

template <class Fn>
void Storage::ForEachTerrainLeafForNode(CountryId const & countryId, Fn && fn) const
{
  bool const leaf = IsLeaf(countryId);
  ForEachCoverageLeaf(countryId, [&](CountryId const & id, TerrainGrid::Coverage const & indices)
  {
    if (leaf || GetInstalledLocalFile(id))
      fn(id, indices);
  });
}

template <class Fn>
void Storage::ForEachTerrainBlockToUpdate(CountryId const & countryId, Fn && fn) const
{
  ForEachCoverageLeaf(countryId, [&](CountryId const & id, TerrainGrid::Coverage const & indices)
  {
    if (!GetInstalledLocalFile(id))
      return;
    for (auto const index : indices)
      if (!IsTerrainOnDisk(index) && !IsTerrainQueued(index))
        fn(id, index);
  });
}

std::vector<uint32_t> Storage::GetTerrainBlocksForNode(CountryId const & countryId) const
{
  std::vector<uint32_t> blocks;
  ForEachTerrainLeafForNode(countryId, [&](CountryId const &, TerrainGrid::Coverage const & indices)
  { blocks.insert(blocks.end(), indices.begin(), indices.end()); });
  base::SortUnique(blocks);
  return blocks;
}

std::vector<uint32_t> Storage::GetCoveringBlocks(CountryId const & countryId) const
{
  // The sorted deduplicated union: the blocks shared by the group leafs must not be
  // counted twice by the size aggregation.
  std::vector<uint32_t> blocks;
  ForEachCoverageLeaf(countryId, [&blocks](CountryId const &, TerrainGrid::Coverage const & indices)
  { blocks.insert(blocks.end(), indices.begin(), indices.end()); });
  base::SortUnique(blocks);
  return blocks;
}

std::string Storage::GetTerrainDir() const
{
  auto terrainDir = GetPlatform().WritableDir();
  if (!m_dataDir.empty())
    terrainDir = base::JoinPath(terrainDir, m_dataDir);
  return base::JoinPath(terrainDir, TERRAIN_DIR);
}

std::vector<terrain::TwmFile> Storage::ScanTerrainFiles() const
{
  std::vector<terrain::TwmFile> scanned;
  for (auto const & [dir, version] : terrain::ListVersionDirs(GetTerrainDir()))
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(dir, TERRAIN_FILE_EXT, files);
    std::sort(files.begin(), files.end());
    for (auto const & name : files)
    {
      auto const path = base::JoinPath(dir, name);
      terrain::TwmFile file;
      auto const result = terrain::TwmSet::ReadFile(path, version, file);
      if (result == terrain::TwmSet::RegResult::Success)
        scanned.push_back(std::move(file));
      else if (result == terrain::TwmSet::RegResult::ObsoleteVersion)
        Platform::RemoveFileIfExists(path);
    }
    Platform::RmDir(dir);  // Removes only empty directories, before downloads are restored.
  }

  return scanned;
}

void Storage::RegisterLocalTerrainFiles(std::vector<terrain::TwmFile> && files)
{
  std::stable_sort(files.begin(), files.end(),
                   [](auto const & a, auto const & b) { return a.m_version > b.m_version; });
  for (auto const & file : files)
  {
    if (base::AnyOf(m_localTerrainFiles, [&](auto const & entry) { return entry.second.m_file.m_path == file.m_path; }))
      continue;
    bool const superseded = base::AnyOf(m_localTerrainFiles, [&](auto const & entry)
    {
      auto const & local = entry.second.m_file;
      return local.m_id == file.m_id || terrain::IsInteriorOverlap(local.m_rect, file.m_rect);
    });
    if (!superseded)
      AddLocalTerrainFile(file);
    else
    {
      LOG(LINFO, ("Removing obsolete terrain", file.m_path));
      Platform::RemoveFileIfExists(file.m_path);
    }
  }
}

std::vector<terrain::TwmFile> Storage::GetLocalTerrainFiles() const
{
  std::vector<terrain::TwmFile> files;
  files.reserve(m_localTerrainFiles.size());
  for (auto const & [id, local] : m_localTerrainFiles)
    files.push_back(local.m_file);
  return files;
}

void Storage::AddLocalTerrainFile(terrain::TwmFile const & file)
{
  auto & local = m_localTerrainFiles[file.m_id];
  local.m_file = file;
  Platform::GetFileSizeByFullPath(file.m_path, local.m_size);
}

void Storage::RegisterTerrainFile(terrain::TwmFile const & file)
{
  // Publishing a new block retires the older files with the same ID or overlapping bounds,
  // including files from a previous grid layout.
  std::vector<terrain::TerrainId> replaced;
  for (auto const & [id, local] : m_localTerrainFiles)
  {
    if (id != file.m_id && !terrain::IsInteriorOverlap(local.m_file.m_rect, file.m_rect))
      continue;
    ASSERT_LESS(local.m_file.m_version, file.m_version, (file.m_path));
    replaced.push_back(id);
  }
  DeleteTerrainFiles(replaced);
  m_terrainRegisterFn(file);
  AddLocalTerrainFile(file);
}

bool Storage::IsTerrainOnDisk(uint32_t index) const
{
  auto const & block = m_terrainGrid.GetBlock(index);
  auto const it = m_localTerrainFiles.find(block.m_id);
  return it != m_localTerrainFiles.end() && it->second.m_file.m_version == block.m_version;
}

bool Storage::IsTerrainQueued(uint32_t index) const
{
  auto const it = m_terrainRequests.find(index);
  return it != m_terrainRequests.end() && it->second.m_state == TerrainRequest::State::Queued;
}

bool Storage::HasQueuedTerrain() const
{
  return base::AnyOf(m_terrainRequests,
                     [](auto const & entry) { return entry.second.m_state == TerrainRequest::State::Queued; });
}

bool Storage::HasOlderTerrain(TerrainGrid::Block const & block) const
{
  return base::AnyOf(m_localTerrainFiles, [&](auto const & entry)
  {
    auto const & file = entry.second.m_file;
    return file.m_version < block.m_version && terrain::IsInteriorOverlap(file.m_rect, block.m_rect);
  });
}

MwmSize Storage::GetDownloadSize(CountriesVec const & countries, bool withTerrain) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  MwmSize size = 0;
  CountriesSet uniqueCountries;
  for (auto const & root : CountriesSet(countries.begin(), countries.end()))
    ForEachInSubtree(root, [&uniqueCountries](CountryId const & id, bool groupNode)
    {
      if (!groupNode)
        uniqueCountries.insert(id);
    });

  for (auto const & country : uniqueCountries)
  {
    CountryTree::Node const * const node = m_countries.FindFirst(country);
    if (node == nullptr || node->ChildrenCount() != 0)
      continue;
    // An out-of-date map is re-downloaded in full (see DownloadNode), so no local-size
    // discount; queued, validating and applying maps are already in the running batch.
    if (GetNodeStatus(*node).status != NodeStatus::OnDisk && !IsCountryInQueue(country) &&
        m_downloadingCountries.count(country) == 0 && !IsDiffApplyingInProgressToCountry(country))
      size += GetCountryFile(country).GetRemoteSize();
  }
  if (withTerrain)
    size += GetTerrainDownloadSize(CountriesVec(uniqueCountries.begin(), uniqueCountries.end()));
  return size;
}

MwmSize Storage::GetTerrainDownloadSize(CountriesVec const & countries) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  std::set<uint32_t> blocks;
  for (auto const & country : countries)
  {
    auto const covering = GetTerrainBlocksForNode(country);
    blocks.insert(covering.begin(), covering.end());
  }
  MwmSize size = 0;
  for (auto const index : blocks)
    if (!IsTerrainOnDisk(index) && !IsTerrainQueued(index))
      size += m_terrainGrid.GetBlock(index).m_size;
  return size;
}

std::string Storage::GetTerrainReadyPath(TerrainGrid::Block const & block) const
{
  return platform::GetFileDownloadPath(block.m_version, m_dataDir, block.m_id, MapFileType::Terrain);
}

void Storage::DeleteTerrainArtifacts(TerrainGrid::Block const & block) const
{
  // The chunked downloader writes exactly these three names, cf. the maps'
  // DeleteDownloaderFilesForCountry; RemoveFileIfExists no-ops on the missing ones.
  std::string const ready = GetTerrainReadyPath(block);
  if (!IsTerrainValidationReading(ready))
    Platform::RemoveFileIfExists(ready);
  Platform::RemoveFileIfExists(ready + RESUME_FILE_EXTENSION);
  Platform::RemoveFileIfExists(ready + DOWNLOADING_FILE_EXTENSION);
}

bool Storage::IsTerrainValidationReading([[maybe_unused]] std::string const & readyPath) const
{
#ifdef OMIM_OS_WINDOWS
  return m_terrainValidationReaders.contains(readyPath);
#else
  // Other platforms unlink a file that a validator keeps reading.
  return false;
#endif
}

void Storage::DownloadTerrain(CountryId const & countryId)
{
  DownloadTerrain(CountriesSet{countryId});
}

void Storage::DownloadTerrain(CountriesSet const & countryIds)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountriesSet changedRegions;
  for (auto const & countryId : countryIds)
    if (EnqueueTerrain(countryId))
      changedRegions.insert(countryId);
  if (changedRegions.empty())
    return;

  SaveDownloadQueue();
  NotifyStatusChangedForHierarchy(changedRegions);
}

bool Storage::EnqueueTerrain(CountryId const & countryId)
{
  auto const covering = GetTerrainBlocksForNode(countryId);
  if (covering.empty())
    return false;
  CountriesSet owners;
  ForEachTerrainLeafForNode(countryId, [&](CountryId const & id, auto const &) { owners.insert(id); });
  size_t scheduled = 0;
  for (auto const index : covering)
  {
    if (IsTerrainOnDisk(index))
      continue;

    auto const & block = m_terrainGrid.GetBlock(index);
    bool const inFlight = IsTerrainQueued(index);
    if (!inFlight)
    {
      // A folder per block version, like the versioned maps folders.
      auto const dir = base::GetDirectory(GetTerrainReadyPath(block));
      if (!Platform::MkDirRecursively(dir))
      {
        // Still notify below: the rows the loop already touched must refresh.
        LOG(LERROR, ("Can't create the terrain directory", dir));
        break;
      }
    }

    auto & request = m_terrainRequests[index];
    for (auto const & owner : owners)
    {
      auto const & indices = m_terrainGrid.GetCoverage().at(owner);
      if (std::binary_search(indices.begin(), indices.end(), index))
        request.m_owners.insert(owner);
    }
    if (inFlight)
      continue;

    request.m_state = TerrainRequest::State::Queued;
    request.m_downloadId = ++m_nextTerrainDownloadId;
    request.m_bytesDownloaded = request.m_lastNotifiedBytes = 0;

    // The terrain block rides the shared downloader queue (FIFO with the maps, the
    // cellular policy applies) as a synthetic country keyed by the block name; the
    // size and the integrity hash come from the grid index like from countries.json.
    // Each immutable block revision selects both its CDN directory and its local
    // folder. Later grids reuse unchanged revisions without downloading them again.
    platform::CountryFile const terrainFile(block.m_id, block.m_size, block.m_hash);
    QueuedCountry queuedBlock(terrainFile, block.m_id, MapFileType::Terrain, block.m_version, m_dataDir,
                              m_diffsDataSource);
    queuedBlock.Subscribe(m_terrainSubscriber);
    if (HasDownloadedFile(queuedBlock.GetFileDownloadPath(), MapFileType::Terrain))
    {
      request.m_bytesDownloaded = block.m_size;
      queuedBlock.OnDownloadFinished(downloader::DownloadStatus::Completed);
    }
    else
      m_downloader->DownloadMapFile(std::move(queuedBlock));
    ++scheduled;
  }
  if (scheduled > 0)
    LOG(LINFO, ("Terrain:", scheduled, "block downloads scheduled for", countryId));
  return true;
}

void Storage::SweepTerrainArtifacts()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // The vast majority never touched the terrain: no directory means nothing to sweep,
  // at the cost of one stat.
  auto const terrainDir = GetTerrainDir();
  if (m_terrainGrid.IsEmpty() || !Platform::IsFileExistsByFullPath(terrainDir))
    return;

  for (auto const & [dir, version] : terrain::ListVersionDirs(terrainDir))
  {
    Platform::FilesList files;
    Platform::GetFilesByRegExp(dir, "\\" TERRAIN_FILE_EXT "\\" READY_FILE_EXTENSION, files);
    for (auto const & file : files)
    {
      std::string const path = base::JoinPath(dir, file);
      // An artifact of a block the downloader is serving right now is not a leftover:
      // the restore re-fires on the Android rendering init (activity recreation).
      auto const index = m_terrainGrid.FindIndex(file.substr(0, file.find('.')));
      if (index && IsTerrainQueued(*index) && path.starts_with(GetTerrainReadyPath(m_terrainGrid.GetBlock(*index))))
        continue;
      if (!IsTerrainValidationReading(path))
        base::DeleteFileX(path);
    }
  }
}

std::set<uint32_t> Storage::GetActiveTerrainBlocks(CountriesSet const & regions, bool includeCompleted) const
{
  // Like the maps, a batch consists of its requests only: the queued and the landed blocks.
  std::set<uint32_t> activeBlocks;
  auto const add = [&](TerrainRequest::State state)
  {
    for (auto const & [index, request] : m_terrainRequests)
    {
      ASSERT(!request.m_owners.empty(), (index));
      if (request.m_state == state &&
          base::AnyOf(request.m_owners, [&](CountryId const & id) { return regions.contains(id); }))
        activeBlocks.insert(index);
    }
  };
  add(TerrainRequest::State::Queued);
  if (!activeBlocks.empty() || includeCompleted)
    add(TerrainRequest::State::Landed);
  return activeBlocks;
}

downloader::Progress Storage::GetTerrainDownloadingProgress(CountryId const & countryId, bool includeCompleted) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (!includeCompleted && !HasQueuedTerrain())
    return {};

  CountriesSet regions;
  for (auto const & [index, request] : m_terrainRequests)
    if (request.m_state != TerrainRequest::State::Failed)
      regions.insert(request.m_owners.begin(), request.m_owners.end());
  if (countryId != GetRootId())
    std::erase_if(regions, [&](CountryId const & id)
    {
      if (id == countryId)
        return false;
      bool inSubtree = false;
      ForEachAncestorExceptForTheRoot(
          id, [&](CountryId const & ancestor, CountryTree::Node const &) { inSubtree |= ancestor == countryId; });
      return !inSubtree;
    });
  return GetTerrainDownloadingProgress(regions, includeCompleted);
}

downloader::Progress Storage::GetTerrainDownloadingProgress(CountriesSet const & regions, bool includeCompleted) const
{
  downloader::Progress progress;
  for (auto const index : GetActiveTerrainBlocks(regions, includeCompleted))
  {
    auto const & block = m_terrainGrid.GetBlock(index);
    progress.m_bytesTotal += block.m_size;
    if (IsTerrainOnDisk(index))
      progress.m_bytesDownloaded += block.m_size;
    else if (auto const & request = m_terrainRequests.at(index); request.m_state == TerrainRequest::State::Queued)
      progress.m_bytesDownloaded += request.m_bytesDownloaded;
  }
  return progress;
}

Storage::TerrainAttrs Storage::GetTerrainAttrs(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  std::vector<uint32_t> covering;
  CountriesSet regions;
  ForEachTerrainLeafForNode(countryId, [&](CountryId const & id, TerrainGrid::Coverage const & indices)
  {
    regions.insert(id);
    covering.insert(covering.end(), indices.begin(), indices.end());
  });
  base::SortUnique(covering);

  TerrainAttrs attrs;
  if (covering.empty())
    return attrs;

  size_t onDisk = 0;
  bool inFlight = false;
  bool hasOlder = false;
  bool hasFailure = false;
  for (auto const index : covering)
  {
    auto const & block = m_terrainGrid.GetBlock(index);
    if (IsTerrainOnDisk(index))
    {
      ++onDisk;
      attrs.m_downloadedSize += block.m_size;
      attrs.m_localSize += m_localTerrainFiles.at(block.m_id).m_size;
      continue;
    }
    hasOlder = hasOlder || HasOlderTerrain(block);
    auto const it = m_terrainRequests.find(index);
    bool const requested = it != m_terrainRequests.end() &&
                           base::AnyOf(it->second.m_owners, [&](CountryId const & id) { return regions.contains(id); });
    if (it != m_terrainRequests.end() && it->second.m_state == TerrainRequest::State::Queued)
    {
      // A block in flight for a neighbour only lands without an update of this node.
      if (requested)
      {
        inFlight = true;
        attrs.m_downloadedSize += it->second.m_bytesDownloaded;
      }
      continue;
    }
    attrs.m_remainingSize += block.m_size;
    hasFailure |= requested && it->second.m_state == TerrainRequest::State::Failed;
  }
  if (hasOlder)
  {
    for (auto const & [id, local] : m_localTerrainFiles)
    {
      auto const & file = local.m_file;
      if (auto const index = m_terrainGrid.FindIndex(id);
          index && m_terrainGrid.GetBlock(*index).m_version == file.m_version)
        continue;
      if (base::AnyOf(covering, [&](uint32_t index)
      { return terrain::IsInteriorOverlap(file.m_rect, m_terrainGrid.GetBlock(index).m_rect); }))
        attrs.m_localSize += local.m_size;
    }
  }
  if (inFlight)
    attrs.m_status = TerrainStatus::Downloading;
  else if (onDisk == covering.size())
    attrs.m_status = TerrainStatus::OnDisk;
  else if (hasFailure)
  {
    attrs.m_status = TerrainStatus::Failed;
    // Like the failed maps (see ParseStatus): every failure auto-retries.
    attrs.m_error = NodeErrorCode::NoInetConnection;
  }
  else if (hasOlder)
    attrs.m_status = TerrainStatus::OnDiskOutOfDate;
  else if (onDisk > 0)
    attrs.m_status = TerrainStatus::Partly;
  else
    attrs.m_status = TerrainStatus::NotDownloaded;
  return attrs;
}

StatusAndError Storage::GetEffectiveStatus(StatusAndError mapStatus, TerrainAttrs const & terrain) const
{
  // The map's own download state and error come first.
  if (mapStatus.status == NodeStatus::NotDownloaded || mapStatus.status <= NodeStatus::Error)
    return mapStatus;
  if (terrain.m_status == TerrainStatus::Downloading)
    return {NodeStatus::Downloading, NodeErrorCode::NoError};
  if (terrain.m_status == TerrainStatus::Failed)
    return {NodeStatus::Error, terrain.m_error};
  if (terrain.m_remainingSize > 0 && mapStatus.status > NodeStatus::OnDiskOutOfDate)
    return {NodeStatus::OnDiskOutOfDate, NodeErrorCode::NoError};
  return mapStatus;
}

std::string DebugPrint(Storage::TerrainStatus status)
{
  switch (status)
  {
  case Storage::TerrainStatus::NotAvailable: return "NotAvailable";
  case Storage::TerrainStatus::NotDownloaded: return "NotDownloaded";
  case Storage::TerrainStatus::Downloading: return "Downloading";
  case Storage::TerrainStatus::Partly: return "Partly";
  case Storage::TerrainStatus::OnDisk: return "OnDisk";
  case Storage::TerrainStatus::Failed: return "Failed";
  case Storage::TerrainStatus::OnDiskOutOfDate: return "OnDiskOutOfDate";
  }
  UNREACHABLE();
}

void Storage::TerrainQueueSubscriber::OnCountryInQueue(QueuedCountry const & queuedCountry)
{
  if (auto const index = m_storage.m_terrainGrid.FindIndex(queuedCountry.GetCountryId()))
    m_storage.NotifyTerrainRegions(*index);
}

void Storage::TerrainQueueSubscriber::OnStartDownloading(QueuedCountry const & queuedCountry)
{
  auto const index = m_storage.m_terrainGrid.FindIndex(queuedCountry.GetCountryId());
  ASSERT(index, (queuedCountry.GetCountryId()));
  auto const owners = m_storage.m_terrainRequests.at(*index).m_owners;
  if (m_storage.m_startDownloadingCallback)
    m_storage.m_startDownloadingCallback();
  m_storage.ReportProgressForHierarchy(owners);
  m_storage.NotifyTerrainRegions(*index);
}

void Storage::TerrainQueueSubscriber::OnDownloadProgress(QueuedCountry const & queuedCountry,
                                                         downloader::Progress const & progress)
{
  m_storage.OnTerrainBlockProgress(queuedCountry, progress);
}

void Storage::TerrainQueueSubscriber::OnDownloadFinished(QueuedCountry const & queuedCountry,
                                                         downloader::DownloadStatus status)
{
  m_storage.OnTerrainBlockDownloaded(queuedCountry, status);
}

void Storage::OnTerrainBlockProgress(QueuedCountry const & queuedCountry, downloader::Progress const & progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const index = m_terrainGrid.FindIndex(queuedCountry.GetCountryId());
  ASSERT(index, (queuedCountry.GetCountryId()));
  // The downloaders report the queued items only.
  auto const it = m_terrainRequests.find(*index);
  ASSERT(it != m_terrainRequests.end() && it->second.m_state == TerrainRequest::State::Queued, (*index));
  auto & request = it->second;
  request.m_bytesDownloaded = progress.m_bytesDownloaded;

  // Terrain rows need only visible changes.
  auto const notifyStep = std::max<uint64_t>(1024 * 1024, queuedCountry.GetDownloadSize() / 64);
  if (progress.m_bytesDownloaded - request.m_lastNotifiedBytes < notifyStep)
    return;
  request.m_lastNotifiedBytes = progress.m_bytesDownloaded;

  // An observer can synchronously cancel and remove the queued item and its owners.
  auto const owners = request.m_owners;
  ReportProgressForHierarchy(owners);
}

void Storage::OnTerrainBlockDownloaded(QueuedCountry const & queuedCountry, downloader::DownloadStatus status)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string const name = queuedCountry.GetCountryId();
  auto const blockIndex = m_terrainGrid.FindIndex(name);
  ASSERT(blockIndex, (name));
  uint32_t const index = *blockIndex;
  auto & request = m_terrainRequests.at(index);
  if (status != downloader::DownloadStatus::Completed)
  {
    LOG(LWARNING, ("Terrain block download failed:", name, status));
    request.m_state = TerrainRequest::State::Failed;
    SaveDownloadQueue();
    NotifyTerrainRegions(index);
    OnFinishDownloading();
    return;
  }

  // Keep an open handle to this attempt's bytes: cancellation can remove the ready
  // path and a restarted download can reuse it before the file-thread task runs.
  std::string const readyPath = queuedCountry.GetFileDownloadPath();
  std::string const finalPath = readyPath.substr(0, readyPath.size() - std::strlen(READY_FILE_EXTENSION));
  std::string const expectedHash = queuedCountry.GetCountryFile().GetHash();
  auto const downloadId = request.m_downloadId;
  std::shared_ptr<base::FileData> file;
  if (m_integrityValidationEnabled)
  {
    file = std::make_shared<base::FileData>(readyPath, base::FileData::Op::READ);
#ifdef OMIM_OS_WINDOWS
    ++m_terrainValidationReaders[readyPath];
#endif
  }
  GetPlatform().RunTask(Platform::Thread::File,
                        [this, index, readyPath, finalPath, expectedHash, downloadId, file = std::move(file)]() mutable
  {
    bool const validated = file != nullptr;
    bool ok = true;
    if (validated)
    {
      // Close the handle before the GUI cleanup below runs.
      SCOPE_GUARD(closeValidation, [&] { file.reset(); });
      coding::Blake3 hasher;
      uint64_t const size = file->Size();
      std::array<uint8_t, 64 * 1024> buffer;
      for (uint64_t offset = 0; offset < size;)
      {
        auto const toRead = std::min<uint64_t>(buffer.size(), size - offset);
        file->Read(offset, buffer.data(), toRead);
        hasher.Update(buffer.data(), toRead);
        offset += toRead;
      }
      ok = hasher.FinalizeToBase64(coding::Blake3::kMwmHashSizeInBytes) == expectedHash;
    }
    GetPlatform().RunTask(Platform::Thread::Gui, [this, index, readyPath, finalPath, downloadId, ok, validated]
    {
#ifdef OMIM_OS_WINDOWS
      if (validated)
      {
        auto const readers = m_terrainValidationReaders.find(readyPath);
        ASSERT(readers != m_terrainValidationReaders.end(), (readyPath));
        if (--readers->second == 0)
          m_terrainValidationReaders.erase(readers);
      }
#else
      UNUSED_VALUE(validated);
#endif
      auto const & block = m_terrainGrid.GetBlock(index);
      auto const it = m_terrainRequests.find(index);
      if (it == m_terrainRequests.end() || it->second.m_state != TerrainRequest::State::Queued ||
          it->second.m_downloadId != downloadId)
      {
        // Cancelled attempts may share this ready file with restarted validators.
        // The last closed reader cleans it only when no new attempt owns the path.
        bool const hasOwner = IsTerrainQueued(index) && GetTerrainReadyPath(block) == readyPath;
        if (!hasOwner && !IsTerrainValidationReading(readyPath))
          Platform::RemoveFileIfExists(readyPath);
        return;
      }
      // Publish only the active attempt. A cancelled validator must never touch a
      // ready or final path now owned by a restarted download.
      bool published = false;
      if (!ok)
      {
        LOG(LWARNING, ("Terrain block integrity check failed:", block.m_id));
        base::DeleteFileX(readyPath);
      }
      else if (base::RenameFileX(readyPath, finalPath))
      {
        Platform::DisableBackupForFile(finalPath);
        terrain::TwmFile file;
        published = terrain::TwmSet::ReadFile(finalPath, block.m_version, file) == terrain::TwmSet::RegResult::Success;
        if (published)
          RegisterTerrainFile(file);
      }
      it->second.m_state = published ? TerrainRequest::State::Landed : TerrainRequest::State::Failed;
      // Publish first, notify after: the observers re-read the attrs synchronously,
      // the landed block must already count as on disk.
      NotifyTerrainRegions(index, published /* allCoverage */);
      SaveDownloadQueue();
      // The drained queue arms the retry and runs the maps' finish work: a failed map never
      // triggers it itself while the terrain trails it in the shared queue.
      OnFinishDownloading();
    });
  });
}

std::set<uint32_t> Storage::GetWantedTerrainBlocks(CountriesSet const & excluded) const
{
  std::set<uint32_t> wanted;
  auto const queued = GetQueuedCountries(m_downloader->GetQueue());
  for (auto const & [region, indices] : m_terrainGrid.GetCoverage())
  {
    if (excluded.contains(region))
      continue;
    // A map retains shared terrain even if another region downloaded it. Validation
    // and diff application retain that coverage after the network queue has drained.
    if (GetInstalledLocalFile(region) || queued.contains(region) || m_downloadingCountries.contains(region) ||
        IsDiffApplyingInProgressToCountry(region))
      wanted.insert(indices.begin(), indices.end());
  }
  // In-flight and failed requests retain their blocks, e.g. the older file of an updated block.
  for (auto const & [index, request] : m_terrainRequests)
    if (request.m_state != TerrainRequest::State::Landed &&
        base::AnyOf(request.m_owners, [&](CountryId const & id) { return !excluded.contains(id); }))
      wanted.insert(index);
  return wanted;
}

CountriesSet Storage::GetFailedTerrainRegions() const
{
  CountriesSet regions;
  for (auto const & [index, request] : m_terrainRequests)
    if (request.m_state == TerrainRequest::State::Failed)
      regions.insert(request.m_owners.begin(), request.m_owners.end());
  return regions;
}

void Storage::NotifyTerrainRegions(uint32_t index, bool allCoverage /* = false */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Shared coverage changes every neighboring row without acquiring request ownership.
  if (allCoverage)
    NotifyStatusChangedForHierarchy(m_terrainGrid.GetRegions(index));
  else if (auto const it = m_terrainRequests.find(index); it != m_terrainRequests.end())
    NotifyStatusChangedForHierarchy(CountriesSet(it->second.m_owners));
}

void Storage::SetDownloadingPolicy(DownloadingPolicy * policy)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloadingPolicy = policy;
  m_downloader->SetDownloadingPolicy(policy);
}

bool Storage::SetDebugMapDownloadServer(std::string const & serverUrl, std::string & normalizedUrl)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!NormalizeDebugMapDownloadServer(serverUrl, normalizedUrl))
    return false;

  settings::Set(kDebugMapDownloadServer, normalizedUrl);
  m_downloader->SetServersList({normalizedUrl});
  return true;
}

void Storage::ResetDebugMapDownloadServer()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  settings::Delete(kDebugMapDownloadServer);
  m_downloader->ResetServersList();
}

bool Storage::GetDebugMapDownloadServer(std::string & serverUrl) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::string storedUrl;
  if (!settings::Get(kDebugMapDownloadServer, storedUrl))
    return false;

  return NormalizeDebugMapDownloadServer(storedUrl, serverUrl);
}

void Storage::DeleteAllLocalMaps(CountriesVec * existedCountries /* = nullptr */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & localFiles : m_localFiles)
  {
    for (auto const & localFile : localFiles.second)
    {
      LOG_SHORT(LINFO, ("Remove:", localFiles.first, DebugPrint(*localFile)));
      if (existedCountries)
        existedCountries->push_back(localFiles.first);
      localFile->SyncWithDisk();
      DeleteFromDiskWithIndexes(*localFile, MapFileType::Map);
      DeleteFromDiskWithIndexes(*localFile, MapFileType::Diff);
    }
  }
}

bool Storage::HaveDownloadedCountries() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return !m_localFiles.empty();
}

void Storage::Clear()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // The terrain blocks are not countries: their leftovers are swept on the queue restore.
  CountriesSet countries;
  m_downloader->GetQueue().ForEachCountry([&countries](QueuedCountry const & country)
  {
    if (country.GetFileType() != MapFileType::Terrain)
      countries.insert(country.GetCountryId());
  });
  for (auto const & [countryId, _] : m_downloadingCountries)
    countries.insert(countryId);

  // Request teardown can write resume state; delete temporary files afterwards.
  m_downloader->Clear();
  m_downloadingCountries.clear();
  for (auto const & countryId : countries)
    DeleteDownloaderFilesForCountry(m_currentVersion, m_dataDir, GetCountryFile(countryId));
  m_justDownloaded.clear();
  m_failedCountries.clear();
  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();
  m_localTerrainFiles.clear();
  m_terrainRequests.clear();
  m_pendingTerrainQueue.emplace();
  SaveDownloadQueue();
}

Storage::WorldStatus Storage::GetForceDownloadWorlds(std::vector<platform::CountryFile> & res) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  bool hasWorld[] = {false, false};
  std::string const worldName[] = {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME};

  {
    // Check if Worlds already present.
    std::vector<platform::LocalCountryFile> localFiles;
    FindAllLocalMapsAndCleanup(m_currentVersion, m_dataDir, localFiles);
    for (auto const & f : localFiles)
    {
      for (int i = 0; i < 2; ++i)
        if (f.GetCountryName() == worldName[i])
          hasWorld[i] = true;
    }

    if (hasWorld[0] && hasWorld[1])
      return WorldStatus::READY;
  }

  Platform & pl = GetPlatform();

  // Parse root data folder (we stored Worlds here in older versions).
  // Note that m_dataDir maybe empty and we take Platform::WritableDir.
  std::vector<platform::LocalCountryFile> rootFiles;
  (void)FindAllLocalMapsInDirectoryAndCleanup(m_dataDir.empty() ? pl.WritableDir() : m_dataDir, 0 /* version */,
                                              -1 /* latestVersion */, rootFiles);

  bool anyWorldWasMoved = false;
  for (auto const & f : rootFiles)
  {
    for (int i = 0; i < 2; ++i)
    {
      if (f.GetCountryName() != worldName[i])
        continue;

      if (!hasWorld[i])
      {
        try
        {
          auto const filePath = f.GetPath(MapFileType::Map);
          uint32_t const version = version::ReadVersionDate(pl.GetReader(filePath, "f"));
          ASSERT(version > 0, ());

          auto const dirPath = base::JoinPath(f.GetDirectory(), std::to_string(version));
          if (pl.MkDirChecked(dirPath) &&
              base::RenameFileX(filePath, base::JoinPath(dirPath, worldName[i] + DATA_FILE_EXTENSION)))
          {
            anyWorldWasMoved = hasWorld[i] = true;
            break;
          }
          else
          {
            LOG(LERROR, ("Can't move", filePath, "into", dirPath));
            return WorldStatus::ERROR_MOVE_FILE;
          }
        }
        catch (RootException const & ex)
        {
          LOG(LERROR, ("Corrupted World file", ex.Msg()));
        }
      }
      DeleteFromDiskWithIndexes(f, MapFileType::Map);
    }
  }

  if (storage::PrepareDirToDownloadCountry(m_currentVersion, m_dataDir).empty())
    return WorldStatus::ERROR_CREATE_FOLDER;

  for (int i = 0; i < 2; ++i)
    if (!hasWorld[i])
      res.push_back(GetCountryFile(worldName[i]));

  return (anyWorldWasMoved && res.empty() ? WorldStatus::WAS_MOVED : WorldStatus::READY);
}

void Storage::RegisterAllLocalMaps(bool enableDiffs /* = false */)
{
  // CHECK_THREAD_CHECKER(m_threadChecker, ());
  // ASSERT(!IsDownloadInProgress(), ());

  m_localFiles.clear();
  m_localFilesForFakeCountries.clear();

  std::vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(m_currentVersion, m_dataDir, localFiles);

  std::sort(localFiles.begin(), localFiles.end(), [](LocalCountryFile const & lhs, LocalCountryFile const & rhs)
  {
    if (lhs.GetCountryFile() != rhs.GetCountryFile())
      return lhs.GetCountryFile() < rhs.GetCountryFile();
    return lhs.GetVersion() > rhs.GetVersion();
  });

  auto i = localFiles.begin();
  while (i != localFiles.end())
  {
    auto j = i + 1;
    while (j != localFiles.end() && i->GetCountryFile() == j->GetCountryFile())
    {
      LocalCountryFile & localFile = *j;
      LOG(LINFO, ("Removing obsolete", localFile));
      localFile.SyncWithDisk();

      DeleteFromDiskWithIndexes(localFile, MapFileType::Map);
      DeleteFromDiskWithIndexes(localFile, MapFileType::Diff);
      ++j;
    }

    RegisterLocalFile(*i);
    i = j;
  }

  FindAllDiffs(m_dataDir, m_notAppliedDiffs);
  // if (enableDiffs)
  //   LoadDiffScheme();
}

void Storage::RegisterAllLocalTerrain()
{
  m_localTerrainFiles.clear();
  RegisterLocalTerrainFiles(ScanTerrainFiles());
}

void Storage::GetLocalMaps(std::vector<LocalFilePtr> & maps) const
{
  // CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (auto const & p : m_localFiles)
    maps.push_back(GetLatestLocalFile(p.first));

  for (auto const & p : m_localFilesForFakeCountries)
    maps.push_back(p.second);

#ifdef DEBUG
  std::sort(maps.begin(), maps.end());
  for (size_t i = 1; i < maps.size(); ++i)
    ASSERT(maps[i - 1] != maps[i], ());
#endif
}

size_t Storage::GetDownloadedFilesCount() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return m_localFiles.size();
}

Country const & Storage::CountryLeafByCountryId(CountryId const & countryId) const
{
  return LeafNodeFromCountryId(m_countries, countryId).Value();
}

Country const & Storage::CountryByCountryId(CountryId const & countryId) const
{
  CountryTree::Node const * node = m_countries.FindFirst(countryId);
  CHECK(node, ("Node with id =", countryId, "not found in country tree."));
  return node->Value();
}

bool Storage::IsNode(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  return m_countries.FindFirst(countryId) != nullptr;
}

bool Storage::IsLeaf(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() == 0;
}

bool Storage::IsInnerNode(CountryId const & countryId) const
{
  if (!IsCountryIdValid(countryId))
    return false;
  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  return node != nullptr && node->ChildrenCount() != 0;
}

LocalAndRemoteSize Storage::CountrySizeInBytes(CountryId const & countryId) const
{
  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  CountryFile const & countryFile = GetCountryFile(countryId);
  LocalAndRemoteSize sizes(0, GetRemoteSize(countryFile));

  if (!IsDiffApplyingInProgressToCountry(countryId))
    sizes.first = localFile ? localFile->GetSize(MapFileType::Map) : 0;

  auto const it = m_downloadingCountries.find(countryId);
  if (it != m_downloadingCountries.cend())
    sizes.first = it->second.m_progress.m_bytesDownloaded;

  return sizes;
}

CountryFile const & Storage::GetCountryFile(CountryId const & countryId) const
{
  return CountryLeafByCountryId(countryId).GetFile();
}

LocalFilePtr Storage::GetLatestLocalFile(CountryId const & countryId) const
{
  // CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const it = m_localFiles.find(countryId);
  if (it != m_localFiles.end())
  {
    // 2 is possible in the moment of updating the map (old and new are both present).
    auto const & files = it->second;
    ASSERT(!files.empty() && files.size() < 3, (files.size()));

    LocalFilePtr latest = files.front();
    for (LocalFilePtr const & file : files)
      if (file->GetVersion() > latest->GetVersion())
        latest = file;

    return latest;
  }
  else
  {
    auto const it = m_localFilesForFakeCountries.find(CountryFile(countryId));
    if (it != m_localFilesForFakeCountries.end())
      return it->second;
  }
  return {};
}

Status Storage::CountryStatus(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // Check if this country has failed while downloading.
  if (m_failedCountries.count(countryId) > 0)
    return Status::DownloadFailed;

  if (m_downloadingCountries.contains(countryId))
    return Status::Downloading;
  if (IsCountryInQueue(countryId))
    return Status::InQueue;

  if (IsDiffApplyingInProgressToCountry(countryId))
    return Status::Applying;

  return Status::UnknownError;
}

namespace
{
/// @param lf Presents on disk and presents in the "old" map.
/// @return True if _really_ obsolete for regions which were not renamed after split.
bool IsRealObsolete(LocalFilePtr const & lf)
{
  /// @todo "Old version" should be somewhere in countries.json, but needs a deep refactoring.
  std::pair<std::string_view, int64_t> constexpr arr[] = {{"China_Guangdong", 260415}};
  for (auto const & e : arr)
    if (lf->GetCountryName() == e.first)
      return lf->GetVersion() < e.second;

  return true;
}
}  // namespace

Status Storage::CountryStatusEx(CountryId const & countryId) const
{
  auto const status = CountryStatus(countryId);
  if (status != Status::UnknownError)
    return status;

  auto const localFile = GetLatestLocalFile(countryId);
  if (!localFile || !(localFile->OnDisk(MapFileType::Map) || localFile->IsInBundle()))
  {
    auto it = m_countriesInfo.m_mwmToOld.find(countryId);
    if (it != m_countriesInfo.m_mwmToOld.end())
    {
      auto const lf = GetLatestLocalFile(it->second);
      if (lf && lf->OnDisk(MapFileType::Map) && IsRealObsolete(lf))
        return Status::OnDiskOutOfDate;
    }
    return Status::NotDownloaded;
  }
  else
  {
    // Check if it is an "old" (outdated) country.
    for (auto const & [_, oldCountry] : m_countriesInfo.m_mwmToOld)
      if (oldCountry == countryId)
      {
        if (IsRealObsolete(localFile))
          return Status::OnDiskOutOfDate;
        else
          break;
      }
  }

  auto const & countryFile = GetCountryFile(countryId);
  if (GetRemoteSize(countryFile) == 0)
    return Status::UnknownError;

  if (localFile->GetVersion() != m_currentVersion)
    return Status::OnDiskOutOfDate;
  return Status::OnDisk;
}

CountriesSet & Storage::GetPendingTerrainQueue()
{
  if (!m_pendingTerrainQueue)
  {
    m_pendingTerrainQueue.emplace();
    std::string saved;
    settings::TryGet(kTerrainDownloadQueueKey, saved);
    strings::Tokenize(saved, ";", [this](std::string_view id) { m_pendingTerrainQueue->emplace(id); });
  }
  return *m_pendingTerrainQueue;
}

void Storage::SaveDownloadQueue()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  std::ostringstream maps;
  auto const append = [&maps](CountryId const & countryId) { maps << (maps.tellp() == 0 ? "" : ";") << countryId; };
  m_downloader->GetQueue().ForEachCountry([&append](QueuedCountry const & country)
  {
    if (country.GetFileType() != MapFileType::Terrain)
      append(country.GetCountryId());
  });
  // A transfer can leave the queue before validation registers its map.
  for (auto const & [countryId, _] : m_downloadingCountries)
    if (!IsCountryInQueue(countryId))
      append(countryId);
  SaveQueueIfChanged(kDownloadQueueKey, maps.str());

  // Terrain requests outlive their maps' queue entries while shared blocks download
  // or validate. Store requesting regions so restore resolves the current grid coverage,
  // including queued blocks that have not created any artifacts yet.
  auto regions = GetPendingTerrainQueue();
  for (auto const & [index, request] : m_terrainRequests)
    if (request.m_state == TerrainRequest::State::Queued)
      regions.insert(request.m_owners.begin(), request.m_owners.end());
  std::ostringstream terrain;
  for (auto const & region : regions)
    terrain << (terrain.tellp() == 0 ? "" : ";") << region;
  SaveQueueIfChanged(kTerrainDownloadQueueKey, terrain.str());
}

void Storage::RestoreDownloadQueue()
{
  std::string download;
  settings::TryGet(kDownloadQueueKey, download);
  auto updatedTerrain = std::exchange(GetPendingTerrainQueue(), {});

  strings::Tokenize(download, ";", [this](std::string_view v)
  {
    if (!base::IsExistIf(m_notAppliedDiffs,
                         [this, v](LocalCountryFile const & localDiff) { return v == FindCountryId(localDiff); }))
    {
      std::string const s(v);
      auto localFile = GetLatestLocalFile(s);
      auto isUpdate = localFile && localFile->OnDisk(MapFileType::Map);
      DownloadNode(s, isUpdate, false /* withTerrain */);
    }
  });
  CountriesSet terrainRegions;
  for (auto const & s : updatedTerrain)
    ForEachCoverageLeaf(s, [&](CountryId const & id, auto const &) { terrainRegions.insert(id); });
  // After the maps: the restored map queue keeps its FIFO priority over the terrain.
  DownloadTerrain(terrainRegions);
  SweepTerrainArtifacts();
}

void Storage::DownloadCountry(CountryId const & countryId, MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (IsCountryInQueue(countryId) || m_downloadingCountries.contains(countryId) ||
      IsDiffApplyingInProgressToCountry(countryId))
    return;

  m_failedCountries.erase(countryId);
  auto const countryFile = GetCountryFile(countryId);
  // If it's not even possible to prepare directory for files before
  // downloading, then mark this country as failed and switch to next
  // country.
  if (!PreparePlaceForCountryFiles(m_currentVersion, m_dataDir, countryFile))
  {
    OnMapDownloadFinished(countryId, DownloadStatus::Failed, type, 0);
    return;
  }

  QueuedCountry queuedCountry(countryFile, countryId, type, m_currentVersion, m_dataDir, m_diffsDataSource);
  queuedCountry.Subscribe(*this);

  if (HasDownloadedFile(GetFileDownloadPath(countryId, type), type))
  {
    OnStartDownloading(queuedCountry);
    SaveDownloadQueue();
    OnDownloadFinished(queuedCountry, DownloadStatus::Completed);
    return;
  }

  m_downloader->DownloadMapFile(std::move(queuedCountry));
}

void Storage::DeleteCountry(CountryId const & countryId, MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_willDelete, ("Storage::Init wasn't called"));

  LocalFilePtr localFile = GetLatestLocalFile(countryId);
  // localFile may be nullptr

  bool const deferredDelete = m_willDelete(countryId, localFile);
  DeleteCountryFiles(countryId, type, deferredDelete);
  DeleteCountryFilesFromDownloader(countryId);
  m_diffsDataSource->RemoveDiffForCountry(countryId);

  NotifyStatusChangedForHierarchy(countryId);
}

bool Storage::DeleteFakeCountry(CountryId const & countryId)
{
  auto const iFile = m_localFilesForFakeCountries.find(CountryFile(countryId));
  if (iFile != m_localFilesForFakeCountries.end())
  {
    auto lf = iFile->second;
    if (!m_willDelete(countryId, lf))
    {
      DeleteCountryIndexes(*lf);
      lf->DeleteFromDisk(MapFileType::Map);

      m_localFilesForFakeCountries.erase(iFile);
    }
    return true;
  }
  return false;
}

void Storage::DeleteCustomCountryVersion(LocalCountryFile const & localFile)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  DeleteFromDiskWithIndexes(localFile, MapFileType::Map);
  DeleteFromDiskWithIndexes(localFile, MapFileType::Diff);

  auto it = m_localFilesForFakeCountries.find(localFile.GetCountryFile());
  if (it != m_localFilesForFakeCountries.end())
    m_localFilesForFakeCountries.erase(it);
}

void Storage::NotifyStatusChanged(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  for (CountryObservers const & observer : m_observers)
    observer.m_changeCountryFn(countryId);
}

void Storage::NotifyStatusChangedForHierarchy(CountryId const & countryId)
{
  // Notification status changing for a leaf in country tree.
  NotifyStatusChanged(countryId);

  // Notification status changing for ancestors in country tree.
  ForEachAncestorExceptForTheRoot(
      countryId, [&](CountryId const & parentId, CountryTree::Node const &) { NotifyStatusChanged(parentId); });
}

void Storage::NotifyStatusChangedForHierarchy(CountriesSet const & countryIds)
{
  CountriesSet notified = countryIds;
  for (auto const & id : countryIds)
    NotifyStatusChanged(id);
  for (auto const & id : countryIds)
    ForEachAncestorExceptForTheRoot(id, [&](CountryId const & parentId, CountryTree::Node const &)
    {
      if (notified.insert(parentId).second)
        NotifyStatusChanged(parentId);
    });
}

bool Storage::IsDownloadInProgress() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  // A downloaded file is still in progress while it is validated and registered.
  return !m_downloader->GetQueue().IsEmpty() || !m_downloadingCountries.empty() || HasQueuedTerrain() ||
         !m_diffsBeingApplied.empty();
}

void Storage::LoadCountriesFile(std::string const & pathToCountriesFile)
{
  if (m_countries.IsEmpty())
  {
    m_currentVersion = LoadCountriesFromFile(pathToCountriesFile, m_countries, m_countriesInfo);
    LOG(LINFO, ("Loaded countries list for version:", m_currentVersion));
    if (m_currentVersion < 0)
      LOG(LERROR, ("Can't load countries file", pathToCountriesFile));
  }
}

int Storage::Subscribe(ChangeCountryFunction change, ProgressFunction progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  int const id = ++m_currentSlotId;
  m_observers.push_back({std::move(change), std::move(progress), id});
  return id;
}

void Storage::Unsubscribe(int slotId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto it = base::FindIf(m_observers, [slotId](auto const & o) { return o.m_slotId == slotId; });
  if (it != m_observers.end())
    m_observers.erase(it);
}

downloader::Progress Storage::GetActiveMapProgress(CountryId const & countryId, bool includeCompleted) const
{
  auto const * scope = m_countries.FindFirst(countryId);
  ASSERT(scope, (countryId));
  if (scope->IsLeaf())
    if (auto const it = m_downloadingCountries.find(countryId); it != m_downloadingCountries.end())
      return it->second.m_progress;

  auto const belongsToNode = [&](CountryId const & id)
  {
    if (scope->IsRoot() || id == countryId)
      return true;
    if (scope->IsLeaf())
      return false;

    // A disputed map can belong to several branches, but contributes only once.
    CountryTree::NodesBufferT nodes;
    m_countries.Find(id, nodes);
    for (auto const * node : nodes)
      while (node->HasParent())
      {
        node = &node->Parent();
        if (node->Value().Name() == countryId)
          return true;
      }
    return false;
  };

  downloader::Progress progress;
  CountriesSet visited;
  auto const shouldAdd = [&](CountryId const & id)
  { return !visited.contains(id) && belongsToNode(id) && visited.insert(id).second; };
  auto const add = [&](downloader::Progress const & bytes)
  {
    progress.m_bytesDownloaded += bytes.m_bytesDownloaded;
    progress.m_bytesTotal += bytes.m_bytesTotal;
  };
  for (auto const & [id, downloading] : m_downloadingCountries)
    if (shouldAdd(id))
      add(downloading.m_progress);
  m_downloader->GetQueue().ForEachCountry([&](QueuedCountry const & country)
  {
    if (country.GetFileType() != MapFileType::Terrain && shouldAdd(country.GetCountryId()))
      add({0, static_cast<int64_t>(country.GetDownloadSize())});
  });
  for (auto const & [id, cancellable] : m_diffsBeingApplied)
    if (shouldAdd(id))
      add(GetMapProgress(id, {}, false /* includeInstalled */));
  if (visited.empty() && !includeCompleted)
    return {};

  // Completed maps remain while either part of the node's download batch is active.
  for (auto const & [id, size] : m_justDownloaded)
    if (shouldAdd(id))
      add({size, size});
  return progress;
}

void Storage::ReportProgressForHierarchy(CountriesSet const & countryIds)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (!base::AnyOf(m_observers, [](auto const & observer) { return static_cast<bool>(observer.m_progressFn); }))
    return;

  CountriesSet visited;
  CountriesVec reported;
  auto const add = [&](CountryId const & id)
  {
    if (visited.insert(id).second)
      reported.push_back(id);
  };
  for (auto const & id : countryIds)
  {
    add(id);
    ForEachAncestorExceptForTheRoot(id, [&](CountryId const & ancestor, CountryTree::Node const &) { add(ancestor); });
  }
  add(GetRootId());

  for (auto const & id : reported)
  {
    // Callbacks can cancel downloads and unsubscribe, so snapshot each node when it is delivered.
    std::vector<ProgressFunction> callbacks;
    for (auto const & observer : m_observers)
      if (observer.m_progressFn)
        callbacks.push_back(observer.m_progressFn);
    if (callbacks.empty())
      return;

    auto const progress = GetDownloadingProgress(id);
    for (auto const & callback : callbacks)
      callback(id, progress);
  }
}

void Storage::OnCountryInQueue(QueuedCountry const & queuedCountry)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  NotifyStatusChangedForHierarchy(queuedCountry.GetCountryId());
  SaveDownloadQueue();
}

void Storage::OnStartDownloading(QueuedCountry const & queuedCountry)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const countryId = queuedCountry.GetCountryId();
  auto const [it, inserted] = m_downloadingCountries.try_emplace(countryId);
  VERIFY(inserted, (countryId));
  it->second.m_progress = {0, static_cast<int64_t>(queuedCountry.GetDownloadSize())};
  if (m_startDownloadingCallback)
    m_startDownloadingCallback();

  ReportProgressForHierarchy({countryId});
  // An observer can cancel the download synchronously.
  if (m_downloadingCountries.contains(countryId))
    NotifyStatusChangedForHierarchy(countryId);
}

void Storage::OnDownloadProgress(QueuedCountry const & queuedCountry, downloader::Progress const & progress)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const countryId = queuedCountry.GetCountryId();
  auto const it = m_downloadingCountries.find(countryId);
  ASSERT(it != m_downloadingCountries.end(), (countryId));
  it->second.m_progress = {progress.m_bytesDownloaded, static_cast<int64_t>(queuedCountry.GetDownloadSize())};
  ReportProgressForHierarchy({countryId});
}

void Storage::OnDownloadFinished(QueuedCountry const & queuedCountry, DownloadStatus status)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const & countryId = queuedCountry.GetCountryId();
  auto const fileType = queuedCountry.GetFileType();
  auto const it = m_downloadingCountries.find(countryId);
  // Deferred transfer completion can arrive after cancellation.
  if (status == DownloadStatus::Completed && it == m_downloadingCountries.end())
    return;

  auto const path = GetFileDownloadPath(countryId, fileType);
  auto finishFn = [this, countryId, fileType, path, size = queuedCountry.GetDownloadSize()](DownloadStatus status)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    // Deletion shares the owner thread with retries, so an old result cannot delete a new file.
    if (status == DownloadStatus::FailedIntegrityCheck)
      base::DeleteFileX(path);
    m_downloadingCountries.erase(countryId);
    OnMapDownloadFinished(countryId, status, fileType, size);
    OnFinishDownloading();
  };

  auto const hash = status == DownloadStatus::Completed && m_integrityValidationEnabled
                      ? GetCountryFile(countryId).GetHash()
                      : std::string{};
  if (!hash.empty())
  {
    auto const size = static_cast<int64_t>(queuedCountry.GetDownloadSize());
    it->second.m_progress = {size, size};
    // Hashing runs on the file thread to keep the GUI responsive; completion returns
    // to Storage's owner thread before accessing its state.

    GetPlatform().RunTask(Platform::Thread::File, [path, hash, lifetime = std::weak_ptr(it->second.m_lifetime),
                                                   fn = std::move(finishFn)]() mutable
    {
      if (lifetime.expired())
        return;
      DownloadStatus status = DownloadStatus::FailedIntegrityCheck;
      std::string readError;
      try
      {
        if (coding::Blake3::CalculateMwmBase64(path) == hash)
          status = DownloadStatus::Completed;
      }
      catch (Reader::Exception const & ex)
      {
        readError = ex.what();
      }

      GetPlatform().RunTask(Platform::Thread::Gui, [lifetime = std::move(lifetime), fn = std::move(fn), path, status,
                                                    readError = std::move(readError)]()
      {
        if (lifetime.expired())
          return;
        // Clean up and notify before error logging can abort a Debug build.
        fn(status);
        if (status == DownloadStatus::Completed)
          LOG(LDEBUG, ("Successful integrity check"));
        else
          LOG(LERROR, ("Integrity check error for", path, readError));
      });
    });
  }
  else
    finishFn(status);
}

void Storage::RegisterDownloadedFiles(CountryId const & countryId, MapFileType type)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const fn = [this, countryId, type](bool isSuccess)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());

    LOG(LINFO, ("Registering downloaded file:", countryId, type, "; success:", isSuccess));

    if (!isSuccess)
    {
      m_justDownloaded.erase(countryId);
      OnMapDownloadFailed(countryId);
      return;
    }

    LocalFilePtr localFile = GetLocalFile(countryId, m_currentVersion);
    ASSERT(localFile, ());

    // This is the final point of success download or diff or update, so better
    // to call DisableBackupForFile here and don't depend from different downloaders.
    Platform::DisableBackupForFile(localFile->GetPath(MapFileType::Map));

    DeleteCountryIndexes(*localFile);
    m_didDownload(countryId, localFile);

    SaveDownloadQueue();

    NotifyStatusChangedForHierarchy(countryId);
  };

  if (type == MapFileType::Diff)
  {
    ApplyDiff(countryId, fn);
    return;
  }
  ASSERT_EQUAL(type, MapFileType::Map, ());

  CountryFile const countryFile = GetCountryFile(countryId);
  LocalFilePtr localFile = GetLocalFile(countryId, m_currentVersion);

  if (!localFile || localFile->IsInBundle())
    localFile = PreparePlaceForCountryFiles(m_currentVersion, m_dataDir, countryFile);
  else
  {
    /// @todo If localFile already exists, we will remove it from disk below?
    LOG(LERROR, ("Downloaded country file for already existing one", *localFile));
  }

  if (!localFile)
  {
    LOG(LERROR, ("Can't prepare LocalCountryFile for", countryFile, "in folder", m_dataDir));
    fn(false);
    return;
  }

  std::string const path = GetFileDownloadPath(countryId, type);
  if (!base::RenameFileX(path, localFile->GetPath(type)))
  {
    /// @todo If localFile already exists (valid), remove it from disk and return false?
    localFile->DeleteFromDisk(type);
    fn(false);
    return;
  }

  // Delete "old" (outdated) country, if any.
  auto const it = m_countriesInfo.m_mwmToOld.find(countryId);
  if (it != m_countriesInfo.m_mwmToOld.end())
  {
    if (!DeleteFakeCountry(it->second))
    {
      // "old" region wasn't renamed
      auto const iFile = m_localFiles.find(it->second);
      if (iFile != m_localFiles.end())
      {
        auto const & lst = iFile->second;
        ASSERT(!lst.empty(), ());
        if (!lst.empty() && IsRealObsolete(lst.front()))
          DeleteCountry(it->second, MapFileType::Map);
      }
    }
  }

  // Register new file.
  RegisterCountryFiles(localFile);
  fn(true);
}

void Storage::OnMapDownloadFinished(CountryId const & countryId, DownloadStatus status, MapFileType type,
                                    uint64_t downloadSize)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(m_didDownload != nullptr, ("Storage::Init wasn't called"));

  if (status != DownloadStatus::Completed)
  {
    if (status == DownloadStatus::FileNotFound && type == MapFileType::Diff)
    {
      AbortDiffScheme();
      NotifyStatusChanged(GetRootId());
    }

    OnMapDownloadFailed(countryId);
    return;
  }

  m_justDownloaded[countryId] = static_cast<int64_t>(downloadSize);
  RegisterDownloadedFiles(countryId, type);
}

/*
void Storage::GetOutdatedCountries(vector<Country const *> & countries) const
{
  for (auto const & p : m_localFiles)
  {
    CountryId const & countryId = p.first;
    LocalFilePtr file = GetLatestLocalFile(countryId);
    if (file && file->GetVersion() != m_currentVersion && !IsWorldCountryID(countryId))
      countries.push_back(&CountryLeafByCountryId(countryId));
  }
}
*/

bool Storage::IsCountryInQueue(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return m_downloader->GetQueue().Contains(countryId);
}

bool Storage::IsDiffApplyingInProgressToCountry(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  return m_diffsBeingApplied.find(countryId) != m_diffsBeingApplied.cend();
}

void Storage::SetDownloaderForTesting(std::unique_ptr<MapFilesDownloader> downloader)
{
  if (m_downloader)
    m_downloader->Clear();

  m_downloader = std::move(downloader);
  m_downloader->SetDownloadingPolicy(m_downloadingPolicy);
}

void Storage::SetEnabledIntegrityValidationForTesting(bool enabled)
{
  m_integrityValidationEnabled = enabled;
}

void Storage::SetCurrentDataVersionForTesting(int64_t currentVersion)
{
  m_currentVersion = currentVersion;
  m_downloader->SetDataVersion(m_currentVersion);
}

void Storage::SetDiffInfoForTesting(diffs::NameDiffInfoMap && diffs)
{
  OnDiffStatusReceived(std::move(diffs));
}

void Storage::SetDownloadingServersForTesting(std::vector<std::string> const & downloadingUrls)
{
  m_downloader->SetServersList(downloadingUrls);
}

void Storage::SetLocaleForTesting(std::string const & jsonBuffer, std::string const & locale)
{
  m_countryNameGetter.SetLocaleForTesting(jsonBuffer, locale);
}

LocalFilePtr Storage::GetLocalFile(CountryId const & countryId, int64_t version) const
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end() || it->second.empty())
    return LocalFilePtr();

  for (auto const & file : it->second)
    if (file->GetVersion() == version)
      return file;
  return LocalFilePtr();
}

void Storage::RegisterCountryFiles(LocalFilePtr localFile)
{
  CHECK(localFile, ());
  localFile->SyncWithDisk();

  CountryId const & countryId = FindCountryId(*localFile);
  LocalFilePtr existingFile = GetLocalFile(countryId, localFile->GetVersion());
  if (existingFile)
    if (existingFile->IsInBundle())
      *existingFile = *localFile;
    else
      ASSERT_EQUAL(localFile.get(), existingFile.get(), ());
  else
    m_localFiles[countryId].push_front(localFile);
}

void Storage::RegisterLocalFile(platform::LocalCountryFile const & localFile)
{
  LocalFilePtr ptr;

  CountryId const & countryId = FindCountryId(localFile);
  if (IsLeaf(countryId))
  {
    ptr = GetLocalFile(countryId, localFile.GetVersion());
    if (!ptr)
    {
      ptr = std::make_shared<LocalCountryFile>(localFile);
      RegisterCountryFiles(ptr);
    }
  }
  else
  {
    ptr = std::make_shared<LocalCountryFile>(localFile);
    ptr->SyncWithDisk();
    m_localFilesForFakeCountries[ptr->GetCountryFile()] = ptr;
  }

  uint64_t const size = ptr->GetSize(MapFileType::Map);
  LOG(LINFO, ("Found file:", countryId, "in directory:", ptr->GetDirectory(), "with size:", size));

  /// Funny, but ptr->GetCountryFile() has valid name only. Size and hash are not initialized.
  /// @todo Store only name (CountryId) in LocalCountryFile instead of CountryFile?
  if (m_currentVersion == ptr->GetVersion() && size != GetCountryFile(countryId).GetRemoteSize())
    LOG(LERROR, ("Inconsistent MWM and version for", *ptr));
}

void Storage::DeleteCountryFiles(CountryId const & countryId, MapFileType type, bool deferredDelete)
{
  auto const it = m_localFiles.find(countryId);
  if (it == m_localFiles.end())
    return;

  if (deferredDelete)
  {
    m_localFiles.erase(countryId);
    return;
  }

  auto & localFiles = it->second;
  for (auto & localFile : localFiles)
  {
    DeleteFromDiskWithIndexes(*localFile, type);
    localFile->SyncWithDisk();
    if (!localFile->HasFiles())
      localFile.reset();
  }
  auto isNull = [](LocalFilePtr const & localFile) { return !localFile; };
  localFiles.remove_if(isNull);
  if (localFiles.empty())
    m_localFiles.erase(countryId);
}

bool Storage::DeleteCountryFilesFromDownloader(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloadingCountries.erase(countryId);
  if (IsDiffApplyingInProgressToCountry(countryId))
    m_diffsBeingApplied[countryId]->Cancel();

  m_downloader->Remove(countryId);
  DeleteDownloaderFilesForCountry(m_currentVersion, m_dataDir, GetCountryFile(countryId));
  SaveDownloadQueue();

  return true;
}

std::string Storage::GetFilePath(CountryId const & countryId, MapFileType type) const
{
  return platform::GetFilePath(m_currentVersion, m_dataDir, countryId, type);
}

std::string Storage::GetFileDownloadPath(CountryId const & countryId, MapFileType type) const
{
  return platform::GetFileDownloadPath(m_currentVersion, m_dataDir, countryId, type);
}

bool Storage::CheckFailedCountries(CountriesVec const & countries) const
{
  for (auto const & country : countries)
    if (m_failedCountries.count(country))
      return true;
  return false;
}

void Storage::RunCountriesCheckAsync()
{
  m_downloader->DownloadAsString(SERVER_DATAVERSION_FILE, [this](std::string const & buffer)
  {
    LOG(LDEBUG, (SERVER_DATAVERSION_FILE, "downloaded"));

    int64_t const dataVersion = ParseIndexAndGetDataVersion(buffer);
    if (dataVersion <= m_currentVersion)
      return false;

    LOG(LDEBUG, ("Try download", COUNTRIES_FILE, "for", dataVersion));

    m_downloader->DownloadAsString(downloader::GetFileDownloadUrl(COUNTRIES_FILE, dataVersion),
                                   [this, dataVersion](std::string const & buffer)
    {
      LOG(LDEBUG, (COUNTRIES_FILE, "downloaded"));

      std::shared_ptr<Storage> storage(new Storage(7 /* dummy */));
      storage->m_currentVersion = LoadCountriesFromBuffer(buffer, storage->m_countries, storage->m_countriesInfo);
      if (storage->m_currentVersion > 0)
      {
        LOG(LDEBUG, ("Apply new version", storage->m_currentVersion, dataVersion));
        ASSERT_EQUAL(storage->m_currentVersion, dataVersion, ());

        /// @todo Or use simple but reliable strategy: download new file and ask to restart the app?
        GetPlatform().RunTask(Platform::Thread::Gui,
                              [this, storage, buffer = std::move(buffer)]() { ApplyCountries(buffer, *storage); });
      }

      return false;
    }, true /* force reset */);

    // True when new download was requested.
    return true;
  }, false /* force reset */);
}

int64_t Storage::ParseIndexAndGetDataVersion(std::string const & index) const
{
  try
  {
    // [ {"start app version" : data version}, ... ]
    glz::generic_u64 root;
    if (auto const error = glz::read_json(root, index); error)
      return 0;

    auto const * array = root.get_if<glz::generic_u64::array_t>();
    if (array == nullptr)
      return 0;

    /// @todo Get correct value somehow ..
    int64_t const appVersion = 21042001;
    int64_t dataVersion = 0;

    for (auto const & item : *array)
    {
      // Make safe parsing here to avoid download errors.
      auto const * object = item.get_if<glz::generic_u64::object_t>();
      if (object == nullptr || object->empty())
        continue;

      auto const & [key, val] = *object->begin();

      int appVer;
      if (val.is_number() && strings::to_int(key, appVer))
      {
        int64_t const dataVer = val.as<int64_t>();
        if (appVersion >= appVer && dataVersion < dataVer)
          dataVersion = dataVer;
      }
    }

    return dataVersion;
  }
  catch (RootException const &)
  {
    return 0;
  }
}

void Storage::ApplyCountries(std::string const & countriesBuffer, Storage & storage)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  /// @todo Or don't skip, but apply new data in OnFinishDownloading().
  if (storage.m_currentVersion <= m_currentVersion || IsDownloadInProgress())
    return;

  {
    // Save file to the WritableDir (LoadCountriesFile checks it first).
    FileWriter writer(base::JoinPath(GetPlatform().WritableDir(), COUNTRIES_FILE));
    writer.Write(countriesBuffer.data(), countriesBuffer.size());
  }

  m_currentVersion = storage.m_currentVersion;
  m_downloader->SetDataVersion(m_currentVersion);

  m_countries = std::move(storage.m_countries);

  /// @todo The best way is to restart the app after ApplyCountries.
  // Do not to update this information containers to avoid possible races.
  // Affiliations, synonyms, etc can be updated with the app update.
  // m_affiliations = std::move(storage.m_affiliations);
  // m_countryNameSynonyms = std::move(storage.m_countryNameSynonyms);

  LOG(LDEBUG, ("Version", m_currentVersion, "is applied"));

  // LoadDiffScheme();

  /// @todo Start World and WorldCoasts download ?!
}

CountryId const Storage::GetRootId() const
{
  return m_countries.GetRoot().Value().Name();
}

void Storage::GetChildren(CountryId const & parent, CountriesVec & childIds) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("CountryId =", parent, "not found in m_countries."));
    return;
  }

  size_t const childrenCount = parentNode->ChildrenCount();
  childIds.clear();
  childIds.reserve(childrenCount);
  for (size_t i = 0; i < childrenCount; ++i)
    childIds.emplace_back(parentNode->Child(i).Value().Name());
}

/*
void Storage::GetLocalRealMaps(CountriesVec & localMaps) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  localMaps.clear();
  localMaps.reserve(m_localFiles.size());

  for (auto const & keyValue : m_localFiles)
    localMaps.push_back(keyValue.first);
}
*/

void Storage::GetChildrenInGroups(CountryId const & parent, CountriesVec & downloadedChildren,
                                  CountriesVec & availChildren, bool keepAvailableChildren) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const parentNode = m_countries.FindFirst(parent);
  if (parentNode == nullptr)
  {
    ASSERT(false, ("CountryId =", parent, "not found in m_countries."));
    return;
  }

  downloadedChildren.clear();
  availChildren.clear();

  // Vector of disputed territories which are downloaded but the other maps
  // in their group are not downloaded. Only mwm in subtree with root == |parent|
  // are taken into account.
  CountriesVec disputedTerritoriesWithoutSiblings;
  // All disputed territories in subtree with root == |parent|.
  CountriesVec allDisputedTerritories;
  parentNode->ForEachChild([&](CountryTree::Node const & childNode)
  {
    CountryId const & childValue = childNode.Value().Name();

    // Do not show bundled World files in Downloader UI, they are always exist and up-to-date.
    if (IsWorldCountryID(childValue))
    {
      auto const pFile = GetLatestLocalFile(childValue);
      if (pFile && pFile->IsInBundle())
        return;
    }

    std::vector<std::pair<CountryId, NodeStatus>> disputedTerritoriesAndStatus;
    StatusAndError const childStatus =
        GetNodeStatusInfo(childNode, disputedTerritoriesAndStatus, true /* isDisputedTerritoriesCounted */);

    ASSERT_NOT_EQUAL(childStatus.status, NodeStatus::Undefined, ());
    for (auto const & disputed : disputedTerritoriesAndStatus)
      allDisputedTerritories.push_back(disputed.first);

    if (childStatus.status == NodeStatus::NotDownloaded)
    {
      availChildren.push_back(childValue);
      for (auto const & disputed : disputedTerritoriesAndStatus)
        if (disputed.second != NodeStatus::NotDownloaded)
          disputedTerritoriesWithoutSiblings.push_back(disputed.first);
    }
    else
    {
      downloadedChildren.push_back(childValue);
      if (keepAvailableChildren)
        availChildren.push_back(childValue);
    }
  });

  [[maybe_unused]] size_t constexpr kAllDisputedCount = 11;
  ASSERT_LESS(disputedTerritoriesWithoutSiblings.size(), kAllDisputedCount, ());
  ASSERT_LESS(allDisputedTerritories.size(), kAllDisputedCount, ());

  CountriesVec uniqueDisputed(disputedTerritoriesWithoutSiblings.begin(), disputedTerritoriesWithoutSiblings.end());
  base::SortUnique(uniqueDisputed);

  for (auto const & countryId : uniqueDisputed)
  {
    // Checks that the number of disputed territories with |countryId| in subtree with root == |parent|
    // is equal to the number of disputed territories with out downloaded sibling
    // with |countryId| in subtree with root == |parent|.
    if (std::count(disputedTerritoriesWithoutSiblings.begin(), disputedTerritoriesWithoutSiblings.end(), countryId) ==
        std::count(allDisputedTerritories.begin(), allDisputedTerritories.end(), countryId))
    {
      // |countryId| is downloaded without any other map in its group.
      downloadedChildren.push_back(countryId);
    }
  }
}

bool Storage::IsNodeDownloaded(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const it = m_localFiles.find(countryId);
  /// @todo IDK what is the logic here, but other functions also check on empty list.
  return (it != m_localFiles.end() && !it->second.empty());
}

bool Storage::HasLatestVersion(CountryId const & countryId) const
{
  return CountryStatusEx(countryId) == Status::OnDisk;
}

bool Storage::IsAllowedToEditVersion(CountryId const & countryId) const
{
  auto const status = CountryStatusEx(countryId);
  switch (status)
  {
  case Status::OnDisk: return true;
  case Status::OnDiskOutOfDate:
  {
    auto const localFile = GetLatestLocalFile(countryId);
    // Under "old" (outdated) countries.
    if (!localFile)
      return false;

    auto const currentVersionTime = base::YYMMDDToSecondsSinceEpoch(static_cast<uint32_t>(m_currentVersion));
    auto const localVersionTime = base::YYMMDDToSecondsSinceEpoch(static_cast<uint32_t>(localFile->GetVersion()));
    return currentVersionTime - localVersionTime < kMaxSecondsTillLastVersionUpdate &&
           base::SecondsSinceEpoch() - localVersionTime < kMaxSecondsTillNoEdits;
  }
  default: return false;
  }
}

int64_t Storage::GetVersion(CountryId const & countryId) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  auto const localMap = GetLatestLocalFile(countryId);
  if (localMap == nullptr)
    return 0;

  return localMap->GetVersion();
}

void Storage::DownloadNode(CountryId const & countryId, bool isUpdate /* = false */, bool withTerrain /* = true */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * node = m_countries.FindFirst(countryId);
  if (!node)
    return;

  LOG(LINFO, ("Downloading", countryId));

  auto downloadAction = [this, isUpdate](CountryTree::Node const & descendantNode)
  {
    if (descendantNode.IsLeaf() && GetNodeStatus(descendantNode).status != NodeStatus::OnDisk)
    {
      auto const & countryId = descendantNode.Value().Name();
      auto const fileType = isUpdate && m_diffsDataSource->HasDiffFor(countryId) ? MapFileType::Diff : MapFileType::Map;

      DownloadCountry(countryId, fileType);
    }
  };

  node->ForEachInSubtree(downloadAction);
  if (withTerrain && !isUpdate)
  {
    CountriesSet regions;
    ForEachCoverageLeaf(countryId, [&](CountryId const & id, auto const &) { regions.insert(id); });
    DownloadTerrain(regions);
  }
}

void Storage::DeleteNode(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);

  if (!node)
    return;

  auto const deleteAction = [this](CountryTree::Node const & descendantNode)
  {
    if (!descendantNode.IsLeaf())
      return;

    auto const & countryId = descendantNode.Value().Name();
    if (m_localFiles.contains(countryId))  // file exists
      DeleteCountry(countryId, MapFileType::Map);
    else
    {
      // when delete "OutOfDate" region based on "old" file
      auto const it = m_countriesInfo.m_mwmToOld.find(countryId);
      if (it != m_countriesInfo.m_mwmToOld.end())
        if (DeleteFakeCountry(it->second))
          NotifyStatusChangedForHierarchy(countryId);
    }
  };
  node->ForEachInSubtree(deleteAction);
  DeleteTerrain(countryId);
}

StatusAndError Storage::GetNodeStatus(CountryTree::Node const & node) const
{
  std::vector<std::pair<CountryId, NodeStatus>> disputedTerritories;
  return GetNodeStatusInfo(node, disputedTerritories, false /* isDisputedTerritoriesCounted */);
}

bool Storage::IsDisputed(CountryTree::Node const & node) const
{
  CountryTree::NodesBufferT found;
  m_countries.Find(node.Value().Name(), found);
  return found.size() > 1;
}

bool Storage::IsCountryLeaf(CountryTree::Node const & node)
{
  return (node.IsLeaf() && !IsWorldCountryID(node.Value().Name()));
}

bool Storage::IsWorldCountryID(CountryId const & country)
{
  return country.starts_with(WORLD_FILE_NAME);
}

/*
void Storage::LoadDiffScheme()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  diffs::LocalMapsInfo localMapsInfo;
  localMapsInfo.m_currentDataVersion = m_currentVersion;

  CountriesVec localMaps;
  GetLocalRealMaps(localMaps);
  for (auto const & countryId : localMaps)
  {
    auto const localFile = GetLatestLocalFile(countryId);
    auto const mapVersion = localFile->GetVersion();

    // mapVersion > m_currentVersion if newer mwm folder was downloaded manually
    if (mapVersion < m_currentVersion && mapVersion > 0)
      localMapsInfo.m_localMaps.emplace(localFile->GetCountryName(), mapVersion);
  }

  if (localMapsInfo.m_localMaps.empty())
  {
    AbortDiffScheme();
    return;
  }

  diffs::Loader::Load(std::move(localMapsInfo), [this](diffs::NameDiffInfoMap && diffs)
  {
    OnDiffStatusReceived(std::move(diffs));
  });
}
*/

void Storage::ApplyDiff(CountryId const & countryId, std::function<void(bool isSuccess)> const & fn)
{
  LOG(LINFO, ("Applying diff for", countryId));

  if (IsDiffApplyingInProgressToCountry(countryId))
    return;

  auto const diffLocalFile = PreparePlaceForCountryFiles(m_currentVersion, m_dataDir, GetCountryFile(countryId));
  uint64_t version;
  if (!diffLocalFile || !m_diffsDataSource->VersionFor(countryId, version))
  {
    fn(false);
    return;
  }

  auto const emplaceResult = m_diffsBeingApplied.emplace(countryId, std::make_unique<base::Cancellable>());
  CHECK_EQUAL(emplaceResult.second, true, ());

  NotifyStatusChangedForHierarchy(countryId);

  diffs::ApplyDiffParams params;
  params.m_diffFile = diffLocalFile;
  params.m_diffReadyPath = GetFileDownloadPath(countryId, MapFileType::Diff);
  params.m_oldMwmFile = GetLocalFile(countryId, version);

  LocalFilePtr & diffFile = params.m_diffFile;
  diffs::ApplyDiff(std::move(params), *emplaceResult.first->second,
                   [this, fn, countryId, diffFile](DiffApplicationResult result)
  {
    CHECK_THREAD_CHECKER(m_threadChecker, ());
    if (result == DiffApplicationResult::Ok && m_integrityValidationEnabled && !diffFile->ValidateIntegrity())
    {
      GetPlatform().RunTask(Platform::Thread::File,
                            [path = diffFile->GetPath(MapFileType::Map)] { base::DeleteFileX(path); });
      result = DiffApplicationResult::Failed;
    }

    if (m_diffsBeingApplied[countryId]->IsCancelled() && result == DiffApplicationResult::Ok)
      result = DiffApplicationResult::Cancelled;

    LOG(LINFO, ("Diff application result for", countryId, ":", result));

    m_diffsBeingApplied.erase(countryId);
    switch (result)
    {
    case DiffApplicationResult::Ok:
    {
      RegisterCountryFiles(diffFile);
      m_diffsDataSource->MarkAsApplied(countryId);
      fn(true);
      break;
    }
    case DiffApplicationResult::Cancelled:
    {
      break;
    }
    case DiffApplicationResult::Failed:
    {
      m_diffsDataSource->RemoveDiffForCountry(countryId);
      fn(false);
      break;
    }
    }

    OnFinishDownloading();
  });
}

void Storage::SetMapSchemeForCountriesWithAbsentDiffs(IsDiffAbsentForCountry const & isAbsent)
{
  std::vector<CountryId> countriesToReplace;
  m_downloader->GetQueue().ForEachCountry([&countriesToReplace, &isAbsent](QueuedCountry const & queuedCountry)
  {
    if (queuedCountry.GetFileType() == MapFileType::Diff && isAbsent(queuedCountry.GetCountryId()))
      countriesToReplace.push_back(queuedCountry.GetCountryId());
  });

  for (auto const & countryId : countriesToReplace)
  {
    DeleteCountryFilesFromDownloader(countryId);
    DownloadCountry(countryId, MapFileType::Map);
  }
}

void Storage::AbortDiffScheme()
{
  SetMapSchemeForCountriesWithAbsentDiffs([](auto const &) { return true; });
  m_diffsDataSource->AbortDiffScheme();
}

/*
bool Storage::IsPossibleToAutoupdate() const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_diffsDataSource->GetStatus() != diffs::Status::Available)
    return false;

  CountriesVec localMaps;
  GetLocalRealMaps(localMaps);
  for (auto const & countryId : localMaps)
  {
    auto const localFile = GetLatestLocalFile(countryId);
    auto const mapVersion = localFile->GetVersion();
    if (mapVersion != m_currentVersion && mapVersion > 0 &&
        !m_diffsDataSource->HasDiffFor(localFile->GetCountryName()))
    {
      return false;
    }
  }

  return true;
}
*/

void Storage::SetStartDownloadingCallback(StartDownloadingCallback const & cb)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_startDownloadingCallback = cb;
}

void Storage::OnFinishDownloading()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (IsDownloadInProgress())
    return;

  m_justDownloaded.clear();
  std::erase_if(m_terrainRequests,
                [](auto const & entry) { return entry.second.m_state == TerrainRequest::State::Landed; });

  // The failed maps and terrain blocks share one auto-retry and its budget.
  auto failed = m_failedCountries;
  auto const failedTerrain = GetFailedTerrainRegions();
  failed.insert(failedTerrain.begin(), failedTerrain.end());
  m_downloadingPolicy->ScheduleRetry(failed, [this](CountriesSet const & needReload)
  {
    for (auto const & country : needReload)
      if (m_failedCountries.contains(country))
        DownloadNode(country, m_diffsDataSource->HasDiffFor(country), false /* withTerrain */);
    DownloadTerrain(GetFailedTerrainRegions());
  });
}

void Storage::OnDiffStatusReceived(diffs::NameDiffInfoMap && diffs)
{
  m_diffsDataSource->SetDiffInfo(std::move(diffs));

  SetMapSchemeForCountriesWithAbsentDiffs([this](auto const & id) { return !m_diffsDataSource->HasDiffFor(id); });

  if (m_diffsDataSource->GetStatus() == diffs::Status::NotAvailable)
    return;

  for (auto const & localDiff : m_notAppliedDiffs)
  {
    auto const countryId = FindCountryIdByFile(localDiff.GetCountryName());

    if (m_diffsDataSource->HasDiffFor(countryId))
      UpdateNode(countryId);
    else
      localDiff.DeleteFromDisk(MapFileType::Diff);
  }

  m_notAppliedDiffs.clear();
}

StatusAndError Storage::GetNodeStatusInfo(CountryTree::Node const & node,
                                          std::vector<std::pair<CountryId, NodeStatus>> & disputedTerritories,
                                          bool isDisputedTerritoriesCounted) const
{
  // Leaf node status.
  if (node.IsLeaf())
  {
    auto const & countryId = node.Value().Name();
    StatusAndError const statusAndError = ParseStatus(CountryStatusEx(countryId));
    if (isDisputedTerritoriesCounted && IsDisputed(node))
      disputedTerritories.push_back(std::make_pair(countryId, statusAndError.status));
    return statusAndError;
  }

  // Group node status.
  NodeStatus result = NodeStatus::NotDownloaded;
  bool allOnDisk = true;

  auto groupStatusCalculator = [&](CountryTree::Node const & nodeInSubtree)
  {
    if (!isDisputedTerritoriesCounted && (!nodeInSubtree.IsLeaf() || result == NodeStatus::Downloading))
      return;

    StatusAndError const statusAndError = ParseStatus(CountryStatusEx(nodeInSubtree.Value().Name()));

    if (isDisputedTerritoriesCounted && IsDisputed(nodeInSubtree))
    {
      disputedTerritories.push_back(std::make_pair(nodeInSubtree.Value().Name(), statusAndError.status));
      return;
    }

    if (result == NodeStatus::Downloading || !nodeInSubtree.IsLeaf())
      return;

    if (statusAndError.status != NodeStatus::OnDisk)
      allOnDisk = false;

    if (static_cast<size_t>(statusAndError.status) < static_cast<size_t>(result))
      result = statusAndError.status;
  };

  node.ForEachDescendant(groupStatusCalculator);
  if (allOnDisk)
    return ParseStatus(Status::OnDisk);
  if (result == NodeStatus::OnDisk)
    return {NodeStatus::Partly, NodeErrorCode::NoError};

  ASSERT_NOT_EQUAL(result, NodeStatus::Undefined, ());
  return {result, NodeErrorCode::NoError};
}

void Storage::GetNodeAttrs(CountryId const & countryId, NodeAttrs & nodeAttrs) const
{
  ASSERT(!countryId.empty(), ());
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::NodesBufferT nodes;
  m_countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));

  // If nodes.size() > 1 countryId corresponds to a disputed territories.
  // In that case it's guaranteed that most of attributes are equal for
  // each element of nodes. See Country class description for further details.
  CountryTree::Node const * const node = nodes[0];

  Country const & nodeValue = node->Value();
  nodeAttrs.m_mwmCounter = nodeValue.GetSubtreeMwmCounter();
  nodeAttrs.m_mwmSize = nodeValue.GetSubtreeMwmSizeBytes();
  StatusAndError statusAndErr = GetNodeStatus(*node);
  nodeAttrs.m_status = statusAndErr.status;
  nodeAttrs.m_error = statusAndErr.error;
  nodeAttrs.m_nodeLocalName = m_countryNameGetter(countryId);
  nodeAttrs.m_nodeLocalDescription = m_countryNameGetter.Get(countryId + LOCALIZATION_DESCRIPTION_SUFFIX);

  bool const includeInstalled = node->IsLeaf() || nodeAttrs.m_status == NodeStatus::OnDisk;
  nodeAttrs.m_downloadingProgress =
      CalculateProgress(*node, GetQueuedCountries(m_downloader->GetQueue()), includeInstalled);

  auto const terrain = GetTerrainAttrs(countryId);
  auto const combinedStatus = GetEffectiveStatus(statusAndErr, terrain);
  nodeAttrs.m_status = combinedStatus.status;
  nodeAttrs.m_error = combinedStatus.error;
  auto const progress = GetDownloadingProgress(countryId).GetCombined();
  if (progress.m_bytesTotal > 0)
    nodeAttrs.m_downloadingProgress = progress;
  else
  {
    nodeAttrs.m_downloadingProgress.m_bytesDownloaded += terrain.m_downloadedSize;
    nodeAttrs.m_downloadingProgress.m_bytesTotal += terrain.m_downloadedSize;
  }
  for (auto const index : GetCoveringBlocks(countryId))
    nodeAttrs.m_mwmSize += m_terrainGrid.GetBlock(index).m_size;

  nodeAttrs.m_downloadSize = 0;
  switch (nodeAttrs.m_status)
  {
  case NodeStatus::Downloading:
  case NodeStatus::Applying:
  case NodeStatus::InQueue: nodeAttrs.m_downloadSize = progress.m_bytesTotal; break;
  case NodeStatus::OnDiskOutOfDate:
  {
    UpdateInfo info;
    GetUpdateInfo(countryId, info);
    nodeAttrs.m_downloadSize = info.m_totalDownloadSizeInBytes;
    break;
  }
  case NodeStatus::NotDownloaded:
  case NodeStatus::Partly: nodeAttrs.m_downloadSize = GetDownloadSize({countryId}); break;
  case NodeStatus::Error:
  {
    auto const failedTerrain = GetFailedTerrainRegions();
    CountriesSet terrainRegions;
    CountriesSet visited;
    ForEachInSubtree(countryId, [&](CountryId const & id, bool groupNode)
    {
      if (groupNode || !visited.insert(id).second)
        return;
      if (m_failedCountries.contains(id))
        nodeAttrs.m_downloadSize += GetRemoteSize(GetCountryFile(id));
      if (failedTerrain.contains(id))
        terrainRegions.insert(id);
    });
    nodeAttrs.m_downloadSize += GetTerrainDownloadSize(CountriesVec(terrainRegions.begin(), terrainRegions.end()));
    break;
  }
  case NodeStatus::OnDisk:
  case NodeStatus::Undefined: break;
  }

  // Local mwm information and information about downloading mwms.
  nodeAttrs.m_localMwmCounter = 0;
  nodeAttrs.m_localMwmSize = terrain.m_localSize;
  nodeAttrs.m_downloadingMwmCounter = 0;
  CountriesSet visitedLocalNodes;
  node->ForEachInSubtree([this, &nodeAttrs, &visitedLocalNodes](CountryTree::Node const & d)
  {
    CountryId const & countryId = d.Value().Name();
    if (!visitedLocalNodes.insert(countryId).second)
      return;

    if (d.IsLeaf())
    {
      // Downloading mwm information.
      auto const status = ParseStatus(CountryStatusEx(countryId)).status;
      ASSERT_NOT_EQUAL(status, NodeStatus::Undefined, ());
      if (status != NodeStatus::NotDownloaded && status != NodeStatus::Partly)
        nodeAttrs.m_downloadingMwmCounter += 1;
    }

    // Local mwm information.
    LocalFilePtr const localFile = GetLatestLocalFile(countryId);
    if (localFile == nullptr)
      return;

    nodeAttrs.m_localMwmCounter += 1;
    nodeAttrs.m_localMwmSize += localFile->GetSize(MapFileType::Map);
  });
  nodeAttrs.m_present = m_localFiles.find(countryId) != m_localFiles.end();

  // Parents information.
  nodeAttrs.m_parentInfo.clear();
  nodeAttrs.m_parentInfo.reserve(nodes.size());
  for (auto const & n : nodes)
  {
    Country const & nValue = n->Value();
    CountryIdAndName countryIdAndName;
    countryIdAndName.m_id = nValue.GetParent();
    if (countryIdAndName.m_id.empty())  // The root case.
      countryIdAndName.m_localName = std::string();
    else
      countryIdAndName.m_localName = m_countryNameGetter(countryIdAndName.m_id);
    nodeAttrs.m_parentInfo.emplace_back(std::move(countryIdAndName));
  }
  // Parents country.
  nodeAttrs.m_topmostParentInfo.clear();
  ForEachAncestorExceptForTheRoot(nodes, [&](CountryId const & ancestorId, CountryTree::Node const & node)
  {
    if (node.Value().GetParent() == GetRootId())
      nodeAttrs.m_topmostParentInfo.push_back({ancestorId, m_countryNameGetter(ancestorId)});
  });
}

void Storage::GetNodeStatuses(CountryId const & countryId, NodeStatuses & nodeStatuses) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  CHECK(node, (countryId));

  auto const statusAndErr = GetEffectiveStatus(GetNodeStatus(*node), GetTerrainAttrs(countryId));
  nodeStatuses.m_status = statusAndErr.status;
  nodeStatuses.m_error = statusAndErr.error;
  nodeStatuses.m_groupNode = (node->ChildrenCount() != 0);
}

void Storage::SetCallbackForClickOnDownloadMap(DownloadFn & downloadFn)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  m_downloadMapOnTheMap = downloadFn;
}

void Storage::DoClickOnDownloadMap(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  if (m_downloadMapOnTheMap)
    m_downloadMapOnTheMap(countryId);
}

downloader::Progress Storage::GetMapProgress(CountryId const & countryId, CountriesSet const & mwmsInQueue,
                                             bool includeInstalled) const
{
  if (auto const it = m_downloadingCountries.find(countryId); it != m_downloadingCountries.end())
    return it->second.m_progress;
  if (mwmsInQueue.count(countryId) != 0)
    return {0, static_cast<int64_t>(GetRemoteSize(GetCountryFile(countryId)))};
  if (auto const it = m_justDownloaded.find(countryId); it != m_justDownloaded.end())
  {
    auto const size = it->second;
    return {size, size};
  }
  if (includeInstalled && CountryStatusEx(countryId) == Status::OnDisk)
  {
    auto const size = static_cast<int64_t>(CountryLeafByCountryId(countryId).GetSubtreeMwmSizeBytes());
    return {size, size};
  }
  return {};
}

downloader::Progress Storage::CalculateProgress(CountryTree::Node const & subtreeRoot, CountriesSet const & mwmsInQueue,
                                                bool includeInstalled) const
{
  downloader::Progress result;
  if (includeInstalled && (!subtreeRoot.IsLeaf() || CountryStatusEx(subtreeRoot.Value().Name()) == Status::OnDisk))
  {
    auto const size = subtreeRoot.Value().GetSubtreeMwmSizeBytes();
    result.m_bytesDownloaded += size;
    result.m_bytesTotal += size;
    return result;
  }

  // A disputed territory can appear more than once in the subtree.
  CountriesSet visited;
  subtreeRoot.ForEachInSubtree([&](CountryTree::Node const & node)
  {
    if (!node.IsLeaf())
      return;
    auto const & id = node.Value().Name();
    if (!visited.insert(id).second)
      return;
    auto const progress = GetMapProgress(id, mwmsInQueue, includeInstalled);
    result.m_bytesDownloaded += progress.m_bytesDownloaded;
    result.m_bytesTotal += progress.m_bytesTotal;
  });
  return result;
}

void Storage::UpdateNode(CountryId const & countryId)
{
  ForEachInSubtree(countryId, [this](CountryId const & descendantId, bool groupNode)
  {
    if (groupNode)
      return;

    bool isDownload = m_localFiles.count(descendantId) > 0;

    if (!isDownload)
    {
      auto it = m_countriesInfo.m_mwmToOld.find(descendantId);
      if (it != m_countriesInfo.m_mwmToOld.end())
      {
        // Check that "old" file is present and OutOfDate. UpdateNode is called unconditionally.
        auto const lf = GetLatestLocalFile(it->second);
        isDownload = lf && lf->OnDisk(MapFileType::Map) && IsRealObsolete(lf);
      }
    }

    if (isDownload)
      DownloadNode(descendantId, true /* isUpdate */);
  });
  CountriesSet regions;
  ForEachTerrainBlockToUpdate(countryId, [&](CountryId const & id, uint32_t) { regions.insert(id); });
  DownloadTerrain(regions);
}

void Storage::CancelDownloadNode(CountryId const & countryId, bool withTerrain /* = true */)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  LOG(LINFO, ("Cancelling the downloading of", countryId));

  auto const setQueue = GetQueuedCountries(m_downloader->GetQueue());

  ForEachInSubtree(countryId, [&](CountryId const & descendantId, bool /* groupNode */)
  {
    auto needNotify = false;
    if (setQueue.count(descendantId) != 0 || m_downloadingCountries.contains(descendantId))
      needNotify = DeleteCountryFilesFromDownloader(descendantId);

    if (m_failedCountries.erase(descendantId) != 0)
      needNotify = true;

    if (needNotify)
      NotifyStatusChangedForHierarchy(descendantId);
  });
  if (withTerrain)
  {
    CancelTerrain(countryId);
    // The blocks that landed for a cancelled new map are left without an owner.
    DeleteUnwantedTerrain(countryId, {} /* excluded */);
  }
  else
    OnFinishDownloading();
}

void Storage::CancelTerrain(CountryId const & countryId)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountriesSet subtree;
  ForEachInSubtree(countryId, [&](CountryId const & id, bool /* groupNode */) { subtree.insert(id); });

  auto & pending = GetPendingTerrainQueue();
  if (!pending.empty())
  {
    // A deletion during startup must also cancel intent that has not been restored yet.
    CountriesSet remaining;
    for (auto const & owner : pending)
    {
      ForEachCoverageLeaf(owner, [&](CountryId const & leaf, auto const &)
      {
        if (!subtree.contains(leaf))
          remaining.insert(leaf);
      });
    }
    pending = std::move(remaining);
  }
  CountriesSet toNotify;
  for (auto it = m_terrainRequests.begin(); it != m_terrainRequests.end();)
  {
    auto & request = it->second;
    if (!base::AnyOf(request.m_owners, [&](CountryId const & id) { return subtree.contains(id); }))
    {
      ++it;
      continue;
    }
    toNotify.insert(request.m_owners.begin(), request.m_owners.end());
    std::erase_if(request.m_owners, [&](CountryId const & id) { return subtree.contains(id); });
    if (!request.m_owners.empty())
    {
      ++it;
      continue;
    }
    // Remove both queued and failed blocks' artifacts so cancellation survives a restart.
    auto const & block = m_terrainGrid.GetBlock(it->first);
    if (request.m_state == TerrainRequest::State::Queued)
      m_downloader->Remove(block.m_id);
    if (request.m_state != TerrainRequest::State::Landed)
      DeleteTerrainArtifacts(block);
    it = m_terrainRequests.erase(it);
  }

  SaveDownloadQueue();
  NotifyStatusChangedForHierarchy(toNotify);
  OnFinishDownloading();
}

void Storage::RetryDownloadNode(CountryId const & countryId)
{
  // Retry each file type separately to preserve map-only routing requests.
  ForEachInSubtree(countryId, [this](CountryId const & descendantId, bool groupNode)
  {
    if (!groupNode && m_failedCountries.count(descendantId) != 0)
    {
      bool const isUpdateRequest = m_diffsDataSource->HasDiffFor(descendantId);
      DownloadNode(descendantId, isUpdateRequest, false /* withTerrain */);
    }
  });
  auto const failed = GetFailedTerrainRegions();
  CountriesSet regions;
  ForEachInSubtree(countryId, [&](CountryId const & id, bool groupNode)
  {
    if (!groupNode && failed.contains(id))
      regions.insert(id);
  });
  DownloadTerrain(regions);
}

bool Storage::GetUpdateInfo(CountryId const & countryId, UpdateInfo & updateInfo) const
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());

  CountriesSet mapOutdated;
  auto const updateInfoAccumulator = [&updateInfo, &mapOutdated, this](CountryTree::Node const & node)
  {
    CountryId const & countryId = node.Value().Name();
    if (!node.IsLeaf() || CountryStatusEx(countryId) != Status::OnDiskOutOfDate)
      return;

    // Disputed maps occur under each of their parents in the country tree.
    if (!mapOutdated.insert(countryId).second)
      return;
    updateInfo.m_numberOfMwmFilesToUpdate += 1;

    LocalAndRemoteSize const sizes = CountrySizeInBytes(countryId);
    updateInfo.m_maxFileSizeInBytes = std::max(updateInfo.m_maxFileSizeInBytes, sizes.second);

    if (m_diffsDataSource->HasDiffFor(countryId))
    {
      uint64_t size;
      m_diffsDataSource->SizeToDownloadFor(countryId, size);
      updateInfo.m_totalDownloadSizeInBytes += size;
    }
    else
      updateInfo.m_totalDownloadSizeInBytes += sizes.second;

    updateInfo.m_sizeDifference += static_cast<int64_t>(sizes.second) - static_cast<int64_t>(sizes.first);
  };

  CountryTree::Node const * const node = m_countries.FindFirst(countryId);
  if (!node)
  {
    ASSERT(false, ());
    return false;
  }
  updateInfo = {};
  node->ForEachInSubtree(updateInfoAccumulator);

  std::set<uint32_t> blocks;
  CountriesSet regions;
  ForEachTerrainBlockToUpdate(countryId, [&](CountryId const & id, uint32_t index)
  {
    blocks.insert(index);
    regions.insert(id);
  });
  for (auto const & id : regions)
    if (!mapOutdated.contains(id))
      ++updateInfo.m_numberOfMwmFilesToUpdate;
  for (auto const index : blocks)
  {
    auto const & block = m_terrainGrid.GetBlock(index);
    updateInfo.m_totalDownloadSizeInBytes += block.m_size;
    updateInfo.m_maxFileSizeInBytes = std::max(updateInfo.m_maxFileSizeInBytes, block.m_size);
    updateInfo.m_sizeDifference += static_cast<int64_t>(block.m_size);
  }
  // Match RegisterTerrainFile: publishing a new block removes older files with
  // the same ID or overlapping bounds, including files from a previous grid layout.
  // Count each old file once when several new blocks replace it.
  for (auto const & [id, local] : m_localTerrainFiles)
    if (base::AnyOf(blocks, [&](uint32_t index)
    {
      auto const & block = m_terrainGrid.GetBlock(index);
      return local.m_file.m_version < block.m_version &&
             (id == block.m_id || terrain::IsInteriorOverlap(local.m_file.m_rect, block.m_rect));
    }))
      updateInfo.m_sizeDifference -= static_cast<int64_t>(local.m_size);

  return true;
}

/// @note No need to call CHECK_THREAD_CHECKER(m_threadChecker, ()) here and below, because
/// we don't change all this containers during update. Consider checking, otherwise.
/// @{
Affiliations const * Storage::GetAffiliations() const
{
  return &m_countriesInfo.m_affiliations;
}

CountryNameSynonyms const & Storage::GetCountryNameSynonyms() const
{
  return m_countriesInfo.m_countryNameSynonyms;
}
/// @}

void Storage::GetTopmostNodesFor(CountryId const & countryId, CountriesVec & nodes, size_t level) const
{
  nodes.clear();
  if (countryId.empty())
    return;

  CountryTree::NodesBufferT treeNodes;
  m_countries.Find(countryId, treeNodes);
  CHECK(!treeNodes.empty(), (countryId));

  nodes.resize(treeNodes.size());
  for (size_t i = 0; i < treeNodes.size(); ++i)
  {
    nodes[i] = countryId;
    CountriesVec path;
    ForEachAncestorExceptForTheRoot(
        {treeNodes[i]}, [&path](CountryId const & id, CountryTree::Node const &) { path.emplace_back(id); });
    if (!path.empty() && level < path.size())
      nodes[i] = path[path.size() - 1 - level];
  }
}

CountryId Storage::GetParentIdFor(CountryId const & countryId) const
{
  ASSERT(!countryId.empty(), ());

  CountryTree::NodesBufferT nodes;
  m_countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));

  if (nodes.size() > 1)
  {
    // Disputed territory. Has multiple parents.
    return {};
  }

  return nodes[0]->Value().GetParent();
}

CountryId Storage::GetTopmostParentFor(CountryId const & countryId) const
{
  return ::storage::GetTopmostParentFor(m_countries, countryId);
}

MwmSize Storage::GetRemoteSize(CountryFile const & file) const
{
  ASSERT(m_diffsDataSource != nullptr, ());
  return storage::GetRemoteSize(*m_diffsDataSource, file);
}

void Storage::OnMapDownloadFailed(CountryId const & countryId)
{
  m_failedCountries.insert(countryId);
  NotifyStatusChangedForHierarchy(countryId);
}
}  // namespace storage
