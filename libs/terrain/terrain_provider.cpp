#include "terrain/terrain_provider.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <memory>

namespace terrain
{
std::vector<TwmFile> TerrainProvider::GetRegisteredFiles() const
{
  std::vector<std::shared_ptr<TwmInfo>> infos;
  m_set.GetInfos(infos);
  std::vector<TwmFile> files;
  files.reserve(infos.size());
  for (auto const & info : infos)
    if (info->IsRegistered())
      files.push_back(info->GetFile());
  return files;
}

bool TerrainProvider::RegisterBlock(TwmFile const & file)
{
  auto const result = m_set.Register(file);
  if (result.second != TwmSet::RegResult::Success && result.second != TwmSet::RegResult::AlreadyRegistered)
  {
    LOG(LWARNING, ("Can't register the terrain block", file.m_path, ":", result.second));
    return false;
  }
  return true;
}

void TerrainProvider::Clear()
{
  m_set.Clear();
}

void TerrainProvider::ReadTerrain(m2::RectD const & rect, int zoom, TerrainData & data,
                                  base::SharedCancellable const & isCancellable) const
{
  data = TerrainData();
  TwmIds ids;
  m_set.GetBlocksByRect(rect, ids);
  if (ids.empty())
    return;

  int const scale = std::min(zoom, scales::GetUpperScale());
  m2::PointD halo(0, 0);
  for (auto const & id : ids)
  {
    auto const handle = m_set.GetHandleById(id);
    if (!handle.IsAlive())
      continue;

    auto const & reader = *handle.GetValue();
    auto const & header = reader.GetHeader();
    if (header.HasRaster(header.GetGeometryIndex(scale)))
    {
      // Raster reads start above the coarsest terrain zoom, whose tile overhang bounds invalidation.
      ASSERT_GREATER(zoom, kMinIsolinesZoom, ());
      auto const extent = reader.GetRasterHalo();
      halo.x = std::max(halo.x, extent.x);
      halo.y = std::max(halo.y, extent.y);
    }
  }

  auto query = rect;
  query.Inflate(halo.x, halo.y);
  if (!query.Intersect(mercator::Bounds::FullRect()))
    return;
  m_set.GetBlocksByRect(query, ids);

  // The blocks decode straight into one mesh: its vertices are deduplicated by the raw quantized points.
  for (auto const & id : ids)
  {
    if (isCancellable.IsCancelled())
      return;

    auto const handle = m_set.GetHandleById(id);
    if (!handle.IsAlive())
      continue;

    auto const & reader = *handle.GetValue();
    reader.ReadTerrain(query, reader.GetHeader().GetGeometryIndex(scale), data, isCancellable);
  }
  data.m_mesh.FinishBuild();
}
}  // namespace terrain
