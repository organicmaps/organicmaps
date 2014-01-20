#pragma once

#include "../indexer/feature_decl.hpp"
#include "../base/mutex.hpp"

#include "../std/set.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"
#include "../std/noncopyable.hpp"

namespace df
{
  struct FeatureInfo
  {
    FeatureInfo(const FeatureID & id)
      : m_id(id), m_isOwner(false) {}

    bool operator < (FeatureInfo const & other) const
    {
      if (!(m_id == other.m_id))
        return m_id < other.m_id;

      return m_isOwner < other.m_isOwner;
    }

    FeatureID m_id;
    bool m_isOwner;
  };

  class MemoryFeatureIndex : private noncopyable
  {
  public:
    void ReadFeaturesRequest(vector<FeatureInfo> & features, vector<size_t> & indexes);
    void RemoveFeatures(vector<FeatureInfo> & features);

  private:
    threads::Mutex m_mutex;
    set<FeatureID> m_features;
  };
}
