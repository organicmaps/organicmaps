#include "drape_frontend/memory_feature_index.hpp"

namespace df
{

void MemoryFeatureIndex::ReadFeaturesRequest(TFeaturesInfo & features, vector<FeatureID> & featuresToRead)
{
  ASSERT(m_isLocked, ());

  for (FeatureInfo & info : features)
  {
    ASSERT(m_features.find(info.m_id) != m_features.end() || !info.m_isOwner,());
    if (!info.m_isOwner && m_features.insert(info.m_id).second)
    {
      featuresToRead.push_back(info.m_id);
      info.m_isOwner = true;
    }
  }
}

void MemoryFeatureIndex::RemoveFeatures(TFeaturesInfo & features)
{
  ASSERT(m_isLocked, ());

  for (FeatureInfo & info : features)
  {
    if (info.m_isOwner)
    {
      VERIFY(m_features.erase(info.m_id) == 1, ());
      info.m_isOwner = false;
    }
  }
}

} // namespace df
