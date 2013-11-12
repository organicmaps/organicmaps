#include "memory_feature_index.hpp"

namespace df
{
  void MemoryFeatureIndex::ReadFeaturesRequest(const vector<FeatureInfo> & features, vector<size_t> & indexes)
  {
    threads::MutexGuard lock(m_mutex);

    for (size_t i = 0; i < features.size(); ++i)
    {
      const FeatureInfo & info = features[i];
      ASSERT(!(m_features.find(info.m_id) == m_features.end() && info.m_isOwner == true), ());
      if (info.m_isOwner == false && m_features.insert(info.m_id).second == true)
        indexes.push_back(i);
    }
  }

  void MemoryFeatureIndex::RemoveFeatures(const vector<FeatureInfo> & features)
  {
    threads::MutexGuard lock(m_mutex);

    for (size_t i = 0; i < features.size(); ++i)
    {
      const FeatureInfo & info = features[i];
      if (info.m_isOwner == true)
        VERIFY(m_features.erase(info.m_id) == 1, ());
    }
  }
}
