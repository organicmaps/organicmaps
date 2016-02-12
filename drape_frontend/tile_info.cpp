#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_info.hpp"

#include "indexer/scales.hpp"

#include "base/scope_guard.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"

namespace df
{

TileInfo::TileInfo(drape_ptr<EngineContext> && context)
  : m_context(move(context))
  , m_is3dBuildings(false)
  , m_isCanceled(false)
{
}

m2::RectD TileInfo::GetGlobalRect() const
{
  return GetTileKey().GetGlobalRect();
}

void TileInfo::ReadFeatureIndex(MapDataProvider const & model)
{
  if (DoNeedReadIndex())
  {
    CheckCanceled();

#ifdef DEBUG
    set<MwmSet::MwmId> existing;
    MwmSet::MwmId lastMwm;
    model.ReadFeaturesID([this, &existing, &lastMwm](FeatureID const & id)
    {
      if (existing.empty() || lastMwm != id.m_mwmId)
      {
        ASSERT(existing.insert(id.m_mwmId).second, ());
        lastMwm = id.m_mwmId;
      }
#else
    model.ReadFeaturesID([this](FeatureID const & id)
    {
#endif
      m_featureInfo.insert(make_pair(id, false));
    }, GetGlobalRect(), GetZoomLevel());
  }
}

void TileInfo::DiscardFeatureInfo(FeatureID const & featureId, MemoryFeatureIndex & memIndex)
{
  CheckCanceled();

  MemoryFeatureIndex::Lock lock(memIndex);
  UNUSED_VALUE(lock);

  m_featureInfo.erase(featureId);
}

bool TileInfo::SetFeatureOwner(FeatureID const & featureId, MemoryFeatureIndex & memIndex)
{
  CheckCanceled();

  MemoryFeatureIndex::Lock lock(memIndex);
  UNUSED_VALUE(lock);

  if (!m_featureInfo[featureId])
  {
    bool isOwner = memIndex.SetFeatureOwner(featureId);
    m_featureInfo[featureId] = isOwner;
    return isOwner;
  }
  return false;
}

void TileInfo::ReadFeatures(MapDataProvider const & model, MemoryFeatureIndex & memIndex)
{
  m_context->BeginReadTile();

  // Reading can be interrupted by exception throwing
  MY_SCOPE_GUARD(ReleaseReadTile, bind(&EngineContext::EndReadTile, m_context.get()));

  vector<FeatureID> featuresToRead;
  {
    MemoryFeatureIndex::Lock lock(memIndex);
    UNUSED_VALUE(lock);

    ReadFeatureIndex(model);
    CheckCanceled();
    memIndex.ReadFeaturesRequest(m_featureInfo, featuresToRead);
  }

  if (!featuresToRead.empty())
  {
    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1, _2),
                      bind(&TileInfo::IsCancelled, this),
                      bind(&TileInfo::SetFeatureOwner, this, _1, ref(memIndex)),
                      bind(&TileInfo::DiscardFeatureInfo, this, _1, ref(memIndex)),
                      model.m_isCountryLoadedByNameFn,
                      make_ref(m_context), m_is3dBuildings);
    model.ReadFeatures(bind<void>(ref(drawer), _1), featuresToRead);
  }
}

void TileInfo::Cancel(MemoryFeatureIndex & memIndex)
{
  m_isCanceled = true;
  MemoryFeatureIndex::Lock lock(memIndex);
  UNUSED_VALUE(lock);
  memIndex.RemoveFeatures(m_featureInfo);
}

bool TileInfo::IsCancelled() const
{
  return m_isCanceled;
}

void TileInfo::InitStylist(FeatureType const & f, Stylist & s)
{
  CheckCanceled();
  df::InitStylist(f, m_context->GetTileKey().m_zoomLevel, m_is3dBuildings, s);
}

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::CheckCanceled() const
{
  if (m_isCanceled)
    MYTHROW(ReadCanceledException, ());
}

int TileInfo::GetZoomLevel() const
{
  return ClipTileZoomByMaxDataZoom(m_context->GetTileKey().m_zoomLevel);
}

} // namespace df
