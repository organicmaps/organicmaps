#include "memory_feature_index.hpp"

namespace df
{
  MemoryFeatureIndex::MemoryFeatureIndex() {}

  void MemoryFeatureIndex::ReadFeaturesRequest(const vector<FeatureInfo> & features, vector<size_t> & indexes)
  {
    threads::MutexGuard lock(m_mutex);

    for (size_t i = 0; i < features.size(); ++i)
    {
      const FeatureInfo & info = features[i];
      bool featureExists = m_features.find(info.m_id) != m_features.end();
      ASSERT(!(featureExists == false && info.m_isOwner == true), ());
      if (info.m_isOwner == false && featureExists == false)
      {
        m_features.insert(info.m_id);
        indexes.push_back(i);
      }
    }
  }

  void MemoryFeatureIndex::RemoveFeatures(const vector<FeatureInfo> & features)
  {
    threads::MutexGuard lock(m_mutex);

    for (size_t i = 0; i < features.size(); ++i)
    {
      const FeatureInfo & info = features[i];
      if (info.m_isOwner == true)
      {
        ASSERT(m_features.find(info.m_id) != m_features.end(), ());
        m_features.erase(info.m_id);
      }
    }
  }
}
