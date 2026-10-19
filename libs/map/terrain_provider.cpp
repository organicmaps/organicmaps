#include "map/terrain_provider.hpp"

#include "geometry/mercator.hpp"
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

bool TerrainProvider::RegisterBlock(TwmFile const & file, m2::RectD * invalidRect)
{
  auto const result = m_set.Register(file);
  if (result.second != TwmSet::RegResult::Success && result.second != TwmSet::RegResult::AlreadyRegistered)
  {
    LOG(LWARNING, ("Can't register the terrain block", file.m_path, ":", result.second));
    return false;
  }
  if (invalidRect)
    AddInvalidationRect(file.m_rect, *invalidRect);
  return true;
}

void TerrainProvider::AddInvalidationRect(m2::RectD const & rect, m2::RectD & invalidRect) const
{
  invalidRect.Add(rect);
  // Neighbor terrain contributes halo normals and contour guide points. Drape's
  // invalidation excludes edge-only tile intersections, so dirty the neighbors
  // explicitly when their previously open boundary gains or loses a surface.
  std::vector<m2::RectD> neighbors;
  m_set.GetBlockRectsByRect(rect, neighbors);
  for (auto const & neighbor : neighbors)
    invalidRect.Add(neighbor);
}

void TerrainProvider::DeleteBlocks(std::vector<TerrainId> const & ids, m2::RectD & invalidRect)
{
  for (auto const & terrainId : ids)
  {
    auto const id = m_set.GetId(terrainId);
    if (!id.IsAlive())
      continue;
    AddInvalidationRect(id.GetInfo()->GetLimitRect(), invalidRect);
    m_set.Deregister(terrainId);
  }
}

void TerrainProvider::Clear()
{
  m_set.Clear();
}

std::optional<double> TerrainProvider::GetElevation(m2::PointD const & point) const
{
  auto const readAt = [&](m2::PointD const & p) -> std::optional<double>
  {
    TwmIds ids;
    m_set.GetBlocksByRect({p, p}, ids);
    for (auto const & id : ids)
    {
      try
      {
        auto const handle = m_set.GetHandleById(id);
        if (!handle.IsAlive())
          continue;
        if (auto elevation = handle.GetValue()->GetReader().GetElevation(p))
          return elevation;
      }
      catch (RootException const & ex)
      {
        LOG(LERROR, ("Error reading terrain block", id, ":", ex.Msg()));
        m_set.Condemn({id});
      }
    }
    return std::nullopt;
  };

  m2::PointD canonical(mercator::WrapX(point.x), point.y);
  auto const elevation = readAt(canonical);
  if (elevation || canonical.x != mercator::Bounds::kMinX)
    return elevation;
  // At the seam either bordering block can be the only downloaded one.
  canonical.x = mercator::Bounds::kMaxX;
  return readAt(canonical);
}

void TerrainProvider::ReadTerrain(m2::RectD const & rect, int zoom, TerrainData & data,
                                  base::SharedCancellable const & isCancellable) const
{
  data = TerrainData();
  TwmIds ids;
  m_set.GetBlocksByRect(rect, ids);
  if (ids.empty())
    return;

  m2::PointD halo(0, 0);
  for (auto const & id : ids)
  {
    if (isCancellable.IsCancelled())
      return;

    try
    {
      auto const handle = m_set.GetHandleById(id);
      if (!handle.IsAlive())
        continue;

      auto const & reader = handle.GetValue()->GetReader();
      auto const & header = reader.GetHeader();
      if (header.HasRaster(header.GetGeometryIndex(std::min(zoom, scales::GetUpperScale()))))
      {
        auto const extent = reader.GetRasterHalo();
        halo.x = std::max(halo.x, extent.x);
        halo.y = std::max(halo.y, extent.y);
      }
    }
    catch (RootException const & ex)
    {
      LOG(LERROR, ("Error reading terrain block", id, ":", ex.Msg()));
      m_set.Condemn({id});
    }
  }

  auto query = rect;
  query.Inflate(halo.x, halo.y);
  if (!query.Intersect(mercator::Bounds::FullRect()))
    return;
  m_set.GetBlocksByRect(query, ids);

  uint8_t coordBits = 0;
  for (auto const & id : ids)
  {
    if (isCancellable.IsCancelled())
      return;

    try
    {
      auto const handle = m_set.GetHandleById(id);
      if (!handle.IsAlive())
        continue;

      auto const & reader = handle.GetValue()->GetReader();
      auto const bits = reader.GetHeader().m_coordBits;
      if (coordBits != 0 && bits != coordBits)
      {
        // Mixed grid snapshots: the dedup keys are raw quantized points, a block with
        // another quantization can't join this mesh - skip it, it is not broken.
        LOG(LWARNING, ("Skipping the terrain block", id, "of another coord bits configuration"));
        continue;
      }
      size_t const geomIndex = reader.GetHeader().GetGeometryIndex(std::min(zoom, scales::GetUpperScale()));
      TerrainData decoded(bits);
      reader.ReadTerrain(query, geomIndex, decoded, isCancellable);

      if (isCancellable.IsCancelled())
        return;
      if (decoded.IsEmpty())
        continue;

      // A failed file must not choose the coordinate configuration of its
      // healthy neighbors. Commit both the data and its bits only after decoding.
      if (coordBits == 0)
      {
        coordBits = bits;
        data = TerrainData(bits);
      }
      data.Append(std::move(decoded));
    }
    catch (RootException const & ex)
    {
      // An unreadable block (e.g. an unsupported newer format) is condemned alone, the
      // neighbor blocks keep rendering. The deregistration is delayed past the handles.
      LOG(LERROR, ("Error reading terrain block", id, ":", ex.Msg()));
      m_set.Condemn({id});
    }
  }
}
}  // namespace terrain
