#include "map/terrain_provider.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <memory>

namespace terrain
{
TerrainProvider::TerrainProvider()
{
  m_set.AddObserver(*this);
}

TerrainProvider::~TerrainProvider()
{
  m_set.RemoveObserver(*this);
}

void TerrainProvider::OnTerrainDeregistered(TwmFile const & file)
{
  if (m_onTerrainDeregistered)
    m_onTerrainDeregistered(file);
}

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

bool TerrainProvider::RegisterBlock(TwmFile const & file, m2::RectD & invalidRect)
{
  auto const result = m_set.Register(file);
  if (result.second != TwmSet::RegResult::Success && result.second != TwmSet::RegResult::AlreadyRegistered)
  {
    LOG(LWARNING, ("Can't register the terrain block", file.m_path, ":", result.second));
    return false;
  }
  invalidRect.Add(file.m_rect);
  return true;
}

void TerrainProvider::DeleteBlocks(std::vector<TerrainId> const & ids, m2::RectD & invalidRect)
{
  for (auto const & terrainId : ids)
  {
    auto const id = m_set.GetId(terrainId);
    if (!id.IsAlive())
      continue;
    invalidRect.Add(id.GetInfo()->GetLimitRect());
    m_set.Deregister(terrainId);
  }
}

void TerrainProvider::Clear()
{
  m_set.Clear();
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
