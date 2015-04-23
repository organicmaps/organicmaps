#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_info.hpp"

#include "indexer/scales.hpp"

#include "base/scope_guard.hpp"

#include "std/bind.hpp"

namespace
{

struct IDsAccumulator
{
  IDsAccumulator(vector<FeatureID> & ids, vector<df::FeatureInfo> const & src)
    : m_ids(ids)
    , m_src(src)
  {
  }

  void operator()(size_t index)
  {
    ASSERT_LESS(index, m_src.size(), ());
    m_ids.push_back(m_src[index].m_id);
  }

  vector<FeatureID> & m_ids;
  vector<df::FeatureInfo> const & m_src;
};

} // namespace

namespace df
{

TileInfo::TileInfo(EngineContext && context)
  : m_context(move(context))
  , m_isCanceled(false)
{}

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
                            MemoryFeatureIndex & memIndex)
{
  m_context.BeginReadTile();

  // Reading can be interrupted by exception throwing
  MY_SCOPE_GUARD(ReleaseReadTile, bind(&EngineContext::EndReadTile, &m_context));

  ReadFeatureIndex(model);

  CheckCanceled();
  vector<size_t> indexes;
  RequestFeatures(memIndex, indexes);

  if (!indexes.empty())
  {
    vector<FeatureID> featuresToRead;
    for_each(indexes.begin(), indexes.end(), IDsAccumulator(featuresToRead, m_featureInfo));

    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2), m_context);
    model.ReadFeatures(ref(drawer), featuresToRead);
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
  //TODO(@kuznetsov): m_context.GetTileKey().m_zoomLevel?
  df::InitStylist(f, GetZoomLevel(), s);
}

//====================================================//

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::RequestFeatures(MemoryFeatureIndex & memIndex, vector<size_t> & featureIndexes)
{
  lock_guard<mutex> lock(m_mutex);
  memIndex.ReadFeaturesRequest(m_featureInfo, featureIndexes);
}

void TileInfo::CheckCanceled() const
{
  if (m_isCanceled)
    MYTHROW(ReadCanceledException, ());
}

int TileInfo::GetZoomLevel() const
{
  int const upperScale = scales::GetUpperScale();
  int const zoomLevel = m_context.GetTileKey().m_zoomLevel;
  return (zoomLevel <= upperScale ? zoomLevel : upperScale);
}

} // namespace df
