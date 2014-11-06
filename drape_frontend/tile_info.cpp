#include "tile_info.hpp"
#include "engine_context.hpp"
#include "stylist.hpp"
#include "rule_drawer.hpp"

#include "../map/feature_vec_model.hpp"

#include "../indexer/scales.hpp"

#include "../base/scope_guard.hpp"

#include "../std/bind.hpp"


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

TileInfo::TileInfo(TileKey const & key)
  : m_key(key)
  , m_isCanceled(false)
{}

m2::RectD TileInfo::GetGlobalRect() const
{
  return m_key.GetGlobalRect();
}

void TileInfo::ReadFeatureIndex(model::FeaturesFetcher const & model)
{
  threads::MutexGuard guard(m_mutex);
  if (DoNeedReadIndex())
  {
    CheckCanceled();
    model.ForEachFeatureID(GetGlobalRect(), *this, GetZoomLevel());
    sort(m_featureInfo.begin(), m_featureInfo.end());
  }
}

void TileInfo::ReadFeatures(model::FeaturesFetcher const & model,
                            MemoryFeatureIndex & memIndex,
                            EngineContext & context)
{
  CheckCanceled();
  vector<size_t> indexes;
  RequestFeatures(memIndex, indexes);

  if (!indexes.empty())
  {
    context.BeginReadTile(m_key);

    // Reading can be interrupted by exception throwing
    MY_SCOPE_GUARD(ReleaseReadTile, bind(&EngineContext::EndReadTile, &context, m_key));

    vector<FeatureID> featuresToRead;
    for_each(indexes.begin(), indexes.end(), IDsAccumulator(featuresToRead, m_featureInfo));

    RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2), m_key, context);
    model.ReadFeatures(drawer, featuresToRead);
  }
}

void TileInfo::Cancel(MemoryFeatureIndex & memIndex)
{
  m_isCanceled = true;
  threads::MutexGuard guard(m_mutex);
  memIndex.RemoveFeatures(m_featureInfo);
}

void TileInfo::operator ()(FeatureID const & id)
{
  m_featureInfo.push_back(id);
  CheckCanceled();
}

void TileInfo::InitStylist(FeatureType const & f, Stylist & s)
{
  CheckCanceled();
  df::InitStylist(f, GetZoomLevel(), s);
}

//====================================================//

bool TileInfo::DoNeedReadIndex() const
{
  return m_featureInfo.empty();
}

void TileInfo::RequestFeatures(MemoryFeatureIndex & memIndex, vector<size_t> & featureIndexes)
{
  threads::MutexGuard guard(m_mutex);
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
  return (m_key.m_zoomLevel <= upperScale ? m_key.m_zoomLevel : upperScale);
}

} // namespace df
