#include "map/terrain_provider.hpp"

#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/tile_key.hpp"

#include "geometry/mercator.hpp"
#include "indexer/scales.hpp"
#include "indexer/terrain/terrain_utils.hpp"

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

void TerrainProvider::AddInvalidationRect(m2::RectD rect, m2::RectD & invalidRect) const
{
  m2::PointD halo;
  {
    std::lock_guard lock(m_readHaloMutex);
    halo = m_maxReadHalo;
  }
  // Every cached tile has read the blocks within its halo (see ReadTerrain), and the edge-touching
  // ones too (the seam normals and contours), which the drape invalidation skips without a margin.
  // A tile also reads the overhang around itself (see df::GetTerrainReadRect), the widest at the
  // coarsest terrain zoom; the mercator tiles are square.
  auto const tile = df::TileKey(0, 0, kMinIsolinesZoom).GetGlobalRect();
  double const overhang = (df::GetTerrainReadRect(tile).SizeX() - tile.SizeX()) / 2;
  double constexpr kEdgeEps = 1e-9;
  rect.Inflate(halo.x + overhang + kEdgeEps, halo.y + overhang + kEdgeEps);
  invalidRect.Add(rect);
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
      auto const handle = m_set.GetHandleById(id);
      if (!handle.IsAlive())
        continue;
      if (auto elevation = handle.GetValue()->GetReader().GetElevation(p))
        return elevation;
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

  int const scale = std::min(zoom, scales::GetUpperScale());
  m2::PointD halo(0, 0);
  for (auto const & id : ids)
  {
    auto const handle = m_set.GetHandleById(id);
    if (!handle.IsAlive())
      continue;

    auto const & reader = handle.GetValue()->GetReader();
    auto const & header = reader.GetHeader();
    if (header.HasRaster(header.GetGeometryIndex(scale)))
    {
      auto const extent = reader.GetRasterHalo();
      halo.x = std::max(halo.x, extent.x);
      halo.y = std::max(halo.y, extent.y);
    }
  }

  {
    std::lock_guard lock(m_readHaloMutex);
    m_maxReadHalo = {std::max(m_maxReadHalo.x, halo.x), std::max(m_maxReadHalo.y, halo.y)};
  }

  auto query = rect;
  query.Inflate(halo.x, halo.y);
  if (!query.Intersect(mercator::Bounds::FullRect()))
    return;
  m_set.GetBlocksByRect(query, ids);

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
    reader.ReadTerrain(query, header.GetGeometryIndex(scale), data, isCancellable);
  }
}
}  // namespace terrain
