#include "tile_info.hpp"
#include "engine_context.hpp"
#include "stylist.hpp"
#include "rule_drawer.hpp"

#include "../map/feature_vec_model.hpp"

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
}

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
    if (DoNeedReadIndex())
    {
      threads::MutexGuard guard(m_mutex);
      CheckCanceled();
      model.ForEachFeatureID(GetGlobalRect(), *this, m_key.m_zoomLevel);
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
      vector<FeatureID> featuresToRead;
      for_each(indexes.begin(), indexes.end(), IDsAccumulator(featuresToRead, m_featureInfo));

      RuleDrawer drawer(bind(&TileInfo::InitStylist, this, _1 ,_2), m_key, context);
      model.ReadFeatures(drawer, featuresToRead);
      context.EndReadTile(m_key);
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

  void TileInfo::InitStylist(const FeatureType & f, Stylist & s)
  {
    CheckCanceled();
    df::InitStylist(f, m_key.m_zoomLevel, s);
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
}
