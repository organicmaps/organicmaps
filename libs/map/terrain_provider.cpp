#include "map/terrain_provider.hpp"

#include "indexer/scales.hpp"
#include "indexer/terrain/terrain_utils.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <set>
#include <vector>

namespace terrain
{

void TerrainProvider::Rescan()
{
  // The blocks live in <dir>/<gridVersion>/ (see Storage::GetTerrainDir); scan the
  // newest version folder, or the flat legacy files when no version folder exists.
  Platform::TFilesWithType subdirs;
  Platform::GetFilesByType(m_dir, Platform::EFileType::Directory, subdirs);
  uint64_t bestVersion = 0;
  for (auto const & [name, type] : subdirs)
  {
    uint64_t version;
    if (strings::to_uint64(name, version) && version > bestVersion)
      bestVersion = version;
  }
  std::string const scanDir = bestVersion > 0 ? base::JoinPath(m_dir, strings::to_string(bestVersion)) : m_dir;

  Platform::FilesList files;
  Platform::GetFilesByExt(scanDir, TERRAIN_FILE_EXT, files);
  std::set<std::string> paths;
  for (auto const & file : files)
    paths.insert(base::JoinPath(scanDir, file));

  size_t registered = 0;
  for (auto const & path : paths)
  {
    auto const result = m_set.Register(path);
    if (result.second == TwmSet::RegResult::Success || result.second == TwmSet::RegResult::AlreadyRegistered)
      ++registered;
    else
      LOG(LWARNING, ("Skipping the terrain file", path, ":", result.second));
  }
  if (registered > 0)
    LOG(LINFO, ("Terrain blocks available:", registered));
}

void TerrainProvider::Clear()
{
  m_set.Clear();
}

void TerrainProvider::ForEachIsoline(m2::RectD const & rect, int zoom, IsolineFn const & fn) const
{
  // TODO(terrain): a tile straddling the +-180 antimeridian keeps its global rect (see
  // TileKey::GetWrappedDataRect), so its beyond-seam half needs the isolines shifted back
  // by the world width. Until then the straddling tiles get the canonical-side isolines only.
  std::vector<TwmId> ids;
  m_set.GetBlocksByRect(rect, ids);
  if (ids.empty())
    return;

  // The handles hand the opened readers off exclusively and keep the blocks alive
  // (a concurrent deregistration is delayed until the last unlock).
  std::vector<TwmSet::Handle> handles;
  std::vector<Reader const *> readers;
  for (auto const & id : ids)
  {
    auto handle = m_set.GetHandleById(id);
    if (!handle.IsAlive())
      continue;
    readers.push_back(&handle.GetValue()->GetReader());
    handles.push_back(std::move(handle));
  }
  if (readers.empty())
    return;

  try
  {
    size_t const geomIndex = readers.front()->GetHeader().GetGeometryIndex(std::min(zoom, scales::GetUpperScale()));
    IsolinesTracer const tracer(readers);
    auto const units = measurement_utils::GetMeasurementUnits();
    tracer.Trace(rect, geomIndex, GetIsolinesStepForZoom(zoom, units), units, fn);
  }
  catch (RootException const & ex)
  {
    // Corrupt data can be detected this late (e.g. inconsistent triangles), condemn the
    // participating blocks so the next queries don't hit it again. The deregistration
    // is delayed past the handles held here.
    LOG(LERROR, ("Condemning the corrupt terrain blocks of the query:", ex.Msg()));
    m_set.Condemn(ids);
  }
}

void TerrainProvider::ForEachTriangles(m2::RectD const & rect, int zoom, TrianglesFn const & fn) const
{
  std::vector<TwmId> ids;
  m_set.GetBlocksByRect(rect, ids);
  if (ids.empty())
    return;

  try
  {
    for (auto const & id : ids)
    {
      auto handle = m_set.GetHandleById(id);
      if (handle.IsAlive())
      {
        auto const & reader = handle.GetValue()->GetReader();
        size_t const geomIndex = reader.GetHeader().GetGeometryIndex(std::min(zoom, scales::GetUpperScale()));
        reader.ForEachFeature(rect, geomIndex, fn);
      }
    }
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Condemning the corrupt terrain blocks of the query:", ex.Msg()));
    m_set.Condemn(ids);
  }
}
}  // namespace terrain
