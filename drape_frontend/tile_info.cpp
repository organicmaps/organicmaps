#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_info.hpp"

#include "drape/texture_manager.hpp"

#include "indexer/scales.hpp"

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"

namespace df
{

TileInfo::TileInfo(drape_ptr<EngineContext> && context)
  : m_context(move(context))
  , m_isCanceled(false)
{
}

m2::RectD TileInfo::GetGlobalRect() const
{
  return GetTileKey().GetGlobalRect();
}

void TileInfo::ReadFeatureIndex(MapDataProvider const & model)
{
  lock_guard<mutex> lock(m_mutex);

  if (DoNeedReadIndex())
  {
    CheckCanceled();
    model.ReadFeaturesID(bind(&TileInfo::ProcessID, this, _1), GetGlobalRect(), GetZoomLevel());
    sort(m_featureInfo.begin(), m_featureInfo.end());
  }
}

void TileInfo::ReadFeatures(MapDataProvider const & model,
                            MemoryFeatureIndex & memIndex,
                            ref_ptr<dp::TextureManager> texMng)
{
  m_context->BeginReadTile();

  // Reading can be interrupted by exception throwing
  MY_SCOPE_GUARD(ReleaseReadTile, bind(&EngineContext::EndReadTile, m_context.get(), texMng));

  ReadFeatureIndex(model);

  CheckCanceled();
  vector<FeatureID> featuresToRead;
  featuresToRead.reserve(AverageFeaturesCount);
  RequestFeatures(memIndex, featuresToRead);

  if (!featuresToRead.empty())
  {
    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2), make_ref(m_context), texMng);
    model.ReadFeatures(bind<void>(ref(drawer), _1), featuresToRead);
  }
}

void TileInfo::Cancel(MemoryFeatureIndex & memIndex)
{
  m_isCanceled = true;
  lock_guard<mutex> lock(m_mutex);
  memIndex.RemoveFeatures(m_featureInfo);
}

void TileInfo::ProcessID(FeatureID const & id)
{
  m_featureInfo.push_back(id);
  CheckCanceled();
}

void TileInfo::InitStylist(FeatureType const & f, Stylist & s)
{
  CheckCanceled();
  df::InitStylist(f, m_context->GetTileKey().m_zoomLevel, s);
}

//====================================================//

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::RequestFeatures(MemoryFeatureIndex & memIndex, vector<FeatureID> & featuresToRead)
{
  lock_guard<mutex> lock(m_mutex);
  memIndex.ReadFeaturesRequest(m_featureInfo, featuresToRead);
}

void TileInfo::CheckCanceled() const
{
  if (m_isCanceled)
    MYTHROW(ReadCanceledException, ());
}

int TileInfo::GetZoomLevel() const
{
  int const upperScale = scales::GetUpperScale();
  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;
  return (zoomLevel <= upperScale ? zoomLevel : upperScale);
}

} // namespace df
