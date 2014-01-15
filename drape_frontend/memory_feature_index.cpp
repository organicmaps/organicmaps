#include "memory_feature_index.hpp"

#include "../base/logging.hpp"

namespace df
{
  void MemoryFeatureIndex::ReadFeaturesRequest(vector<FeatureInfo> & features, vector<size_t> & indexes)
  {
    threads::MutexGuard lock(m_mutex);

    for (size_t i = 0; i < features.size(); ++i)
    {
      FeatureInfo & info = features[i];
      if (info.m_isOwner == false && m_features.insert(info.m_id).second == true)
      {
        info.m_isOwner = true;
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
        //VERIFY(m_features.erase(info.m_id) == 1, ("Erase of ", info.m_id.m_mwm, " ", info.m_id.m_offset));
        m_features.erase(info.m_id);
    }
  }
}
