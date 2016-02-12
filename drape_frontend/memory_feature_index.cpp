#include "drape_frontend/memory_feature_index.hpp"

namespace df
{

void MemoryFeatureIndex::ReadFeaturesRequest(TFeaturesInfo const & features, vector<FeatureID> & featuresToRead)
{
  ASSERT(m_isLocked, ());

  for (auto const & featureInfo : features)
  {
    if (m_features.find(featureInfo.first) == m_features.end())
      featuresToRead.push_back(featureInfo.first);
  }
}

bool MemoryFeatureIndex::SetFeatureOwner(FeatureID const & feature)
{
  ASSERT(m_isLocked, ());

  return m_features.insert(feature).second;
}

void MemoryFeatureIndex::RemoveFeatures(TFeaturesInfo & features)
{
  ASSERT(m_isLocked, ());

  for (auto & featureInfo : features)
  {
    if (featureInfo.second)
    {
      VERIFY(m_features.erase(featureInfo.first) == 1, ());
      featureInfo.second = false;
    }
  }
}

} // namespace df
