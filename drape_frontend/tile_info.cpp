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
    model.ReadFeaturesID(bind(&TileInfo::ProcessID, this, _1), GetGlobalRect(), GetZoomLevel());

    //sort(m_featureInfo.begin(), m_featureInfo.end());
    // Do debug check instead of useless sorting.
#ifdef DEBUG
    set<MwmSet::MwmId> existing;
    auto i = m_featureInfo.begin();
    while (i != m_featureInfo.end())
    {
      auto const & id = i->m_id.m_mwmId;
      ASSERT(existing.insert(id).second, ());
      i = find_if(i+1, m_featureInfo.end(), [&id](FeatureInfo const & info)
      {
        return (id != info.m_id.m_mwmId);
      });
    }
#endif
  }
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
    featuresToRead.reserve(kAverageFeaturesCount);
    memIndex.ReadFeaturesRequest(m_featureInfo, featuresToRead);
  }

  if (!featuresToRead.empty())
  {
    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2),
                      bind(&TileInfo::IsCancelled, this),
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

void TileInfo::ProcessID(FeatureID const & id)
{
  m_featureInfo.push_back(id);
}

void TileInfo::InitStylist(FeatureType const & f, Stylist & s)
{
  CheckCanceled();
  df::InitStylist(f, m_context->GetTileKey().m_styleZoomLevel, m_is3dBuildings, s);
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
  ASSERT_LESS_OR_EQUAL(m_context->GetTileKey().m_zoomLevel, scales::GetUpperScale(), ());
  return m_context->GetTileKey().m_zoomLevel;
}

} // namespace df
