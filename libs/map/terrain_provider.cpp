#include "map/terrain_provider.hpp"

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

  // The blocks decode straight into one mesh: its vertices are deduplicated by the raw
  // quantized points, so the first block fixes the coordinate configuration.
  uint8_t coordBits = 0;
  for (auto const & id : ids)
  {
    if (isCancellable.IsCancelled())
      return;

    auto const handle = m_set.GetHandleById(id);
    if (!handle.IsAlive())
      continue;

    auto const & reader = handle.GetValue()->GetReader();
    auto const & header = reader.GetHeader();
    if (coordBits == 0)
    {
      coordBits = header.m_coordBits;
      data = TerrainData(coordBits);
    }
    else if (header.m_coordBits != coordBits)
    {
      LOG(LWARNING, ("Skipping the terrain block", id, "of another coord bits configuration"));
      continue;
    }
    reader.ReadTerrain(rect, header.GetGeometryIndex(scale), data, isCancellable);
  }
}
}  // namespace terrain
