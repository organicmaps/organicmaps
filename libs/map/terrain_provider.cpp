#include "map/terrain_provider.hpp"

#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

namespace terrain
{

namespace
{
// The block data version = the name of its version folder (cf. Storage::GetTerrainDir);
// the flat legacy files get 0.
int64_t VersionFromPath(std::string const & path)
{
  uint64_t version;
  if (strings::to_uint64(base::FileNameFromFullPath(base::GetDirectory(path)), version))
    return static_cast<int64_t>(version);
  return 0;
}

void RemoveDirIfEmpty(std::string const & dir)
{
  Platform::FilesList left;
  Platform::GetFilesByExt(dir, TERRAIN_FILE_EXT, left);
  if (left.empty() && Platform::RmDir(dir) == Platform::ERR_OK)
    LOG(LINFO, ("Removed the emptied terrain folder", dir));
}
}  // namespace

void TerrainProvider::Rescan()
{
  // Every version folder renders together (plus the flat legacy files as the version 0):
  // the newest-first registration order makes "the newest data wins" automatic via the
  // TwmSet overlap rejection, and the older non-overlapping blocks keep rendering -
  // stale terrain beats no terrain. Cf. the maps: the same policy as the MwmSet
  // keeping the latest version of every country.
  auto const dirs = ListVersionDirs(m_dir);

  int64_t const newestVersion = dirs.front().m_version;
  size_t registered = 0, replaced = 0;
  for (auto const & [dir, version] : dirs)
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(dir, TERRAIN_FILE_EXT, files);
    std::sort(files.begin(), files.end());
    bool emptied = false;
    for (auto const & file : files)
    {
      std::string const path = base::JoinPath(dir, file);
      auto const result = m_set.Register(path, version);
      if (result.second == TwmSet::RegResult::Success || result.second == TwmSet::RegResult::AlreadyRegistered)
        ++registered;
      else if (result.second == TwmSet::RegResult::Overlapping && version < newestVersion)
      {
        // An older block under the newer coverage: the newer download already replaced
        // it (see OnBlockDownloaded), only the file survived a restart - finish the job.
        base::DeleteFileX(path);
        emptied = true;
        ++replaced;
      }
      else if (result.second == TwmSet::RegResult::ObsoleteVersion)
      {
        // An old format file no build reads anymore: the registration read its header
        // only. The downloader refetches the regenerated block (its versioned path
        // differs, see Storage::OnTerrainScanned), so free the space right away.
        LOG(LINFO, ("Deleting the obsolete terrain file", path));
        base::DeleteFileX(path);
        emptied = true;
        ++replaced;
      }
      else
        LOG(LWARNING, ("Skipping the terrain file", path, ":", result.second));
    }
    if (emptied && dir != m_dir)
      RemoveDirIfEmpty(dir);
  }
  if (registered > 0 || replaced > 0)
    LOG(LINFO, ("Terrain blocks available:", registered, "outdated deleted:", replaced));
  m_scanned = true;
}

std::vector<TwmFile> TerrainProvider::GetRegisteredFiles() const
{
  std::vector<std::shared_ptr<TwmInfo>> infos;
  m_set.GetInfos(infos);
  std::vector<TwmFile> files;
  files.reserve(infos.size());
  for (auto const & info : infos)
  {
    if (info->IsRegistered())
    {
      uint64_t size = 0;
      Platform::GetFileSizeByFullPath(info->GetFilePath(), size);
      files.push_back({base::FilenameWithoutExt(base::FileNameFromFullPath(info->GetFilePath())), info->GetVersion(),
                       info->GetLimitRect(), size});
    }
  }
  return files;
}

void TerrainProvider::OnBlockDownloaded(std::string const & path, m2::RectD & invalidRect)
{
  m2::RectD newRect;
  if (!TwmSet::ReadLimitRect(path, newRect))
    return;
  int64_t const version = VersionFromPath(path);

  // The newer coverage replaces the older blocks it intersects, even partially (the
  // remainder of a partially covered old block is reported as OnDiskOutOfDate until
  // its region is updated too). The deregistration is delayed past the running
  // queries; deleting an open file is safe on POSIX.
  DeleteBlocksImpl({newRect}, [&](TwmInfo const & info)
  { return info.GetVersion() < version && info.GetFilePath() != path; }, invalidRect);

  auto const result = m_set.Register(path, version);
  if (result.second != TwmSet::RegResult::Success && result.second != TwmSet::RegResult::AlreadyRegistered)
    LOG(LWARNING, ("Can't register the downloaded terrain block", path, ":", result.second));
  invalidRect.Add(newRect);
}

void TerrainProvider::DeleteBlocks(std::vector<m2::RectD> const & rects, m2::RectD & invalidRect)
{
  DeleteBlocksImpl(rects, [](TwmInfo const &) { return true; }, invalidRect);
}

void TerrainProvider::DeleteBlocksImpl(std::vector<m2::RectD> const & rects,
                                       std::function<bool(TwmInfo const &)> const & pred, m2::RectD & invalidRect)
{
  std::map<std::string, m2::RectD> blocks;
  std::vector<TwmId> ids;
  for (auto const & rect : rects)
  {
    m_set.GetBlocksByRect(rect, ids);
    for (auto const & id : ids)
    {
      auto const & info = *id.GetInfo();
      // GetBlocksByRect intersection is inclusive: an edge-touching neighbor is not
      // covered by the rect, keep it.
      if (IsInteriorOverlap(rect, info.GetLimitRect()) && pred(info))
        blocks.emplace(info.GetFilePath(), info.GetLimitRect());
    }
  }

  std::set<std::string> emptiedDirs;
  for (auto const & [path, rect] : blocks)
  {
    invalidRect.Add(rect);
    m_set.Deregister(path);
    base::DeleteFileX(path);
    std::string dir = base::GetDirectory(path);
    if (dir != m_dir)
      emptiedDirs.insert(std::move(dir));
    LOG(LINFO, ("Deleted the terrain block", path));
  }
  for (auto const & dir : emptiedDirs)
    RemoveDirIfEmpty(dir);
}

void TerrainProvider::Clear()
{
  m_set.Clear();
  m_scanned = false;
}

bool IsTerrainDrawableAt(storage::CountryInfoGetter const & infoGetter, IsCountryLoadedFn const & isLoaded,
                         m2::PointD const & tileCenter)
{
  auto const countryId = infoGetter.GetRegionCountryId(tileCenter);
  return countryId.empty() || isLoaded(countryId);
}

void TerrainProvider::ReadMesh(m2::RectD const & rect, int zoom, TileMesh & mesh) const
{
  std::vector<TwmId> ids;
  m_set.GetBlocksByRect(rect, ids);
  if (ids.empty())
    return;

  uint8_t coordBits = 0;
  for (auto const & id : ids)
  {
    try
    {
      auto const handle = m_set.GetHandleById(id);
      if (!handle.IsAlive())
        continue;
      auto const & reader = handle.GetValue()->GetReader();
      if (coordBits == 0)
      {
        coordBits = reader.GetHeader().m_coordBits;
        mesh = TileMesh(coordBits);
      }
      else if (reader.GetHeader().m_coordBits != coordBits)
      {
        // Mixed grid snapshots: the dedup keys are raw quantized points, a block with
        // another quantization can't join this mesh - skip it, it is not broken.
        LOG(LWARNING, ("Skipping the terrain block", id, "of another coord bits configuration"));
        continue;
      }
      size_t const geomIndex = reader.GetHeader().GetGeometryIndex(std::min(zoom, scales::GetUpperScale()));
      reader.ReadMesh(rect, geomIndex, mesh);
    }
    catch (RootException const & ex)
    {
      // An unreadable block (e.g. an unsupported newer format) is condemned alone, the
      // neighbor blocks keep rendering. The deregistration is delayed past the handles.
      LOG(LERROR, ("Condemning the unreadable terrain block", id, ":", ex.Msg()));
      m_set.Condemn({id});
    }
  }
}
}  // namespace terrain
