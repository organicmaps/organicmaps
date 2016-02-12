#pragma once

#include "indexer/feature_decl.hpp"

#include "base/buffer_vector.hpp"
#include "base/mutex.hpp"

#include "std/set.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/noncopyable.hpp"

namespace df
{

using TFeaturesInfo = map<FeatureID, bool>;

class MemoryFeatureIndex : private noncopyable
{
public:
  class Lock
  {
    threads::MutexGuard lock;
    MemoryFeatureIndex & m_index;
  public:
    Lock(MemoryFeatureIndex & index)
      : lock(index.m_mutex)
      , m_index(index)
    {
      m_index.m_isLocked = true;
    }

    ~Lock()
    {
      m_index.m_isLocked = false;
    }
  };

  void ReadFeaturesRequest(const TFeaturesInfo & features, vector<FeatureID> & featuresToRead);
  bool SetFeatureOwner(const FeatureID & feature);
  void RemoveFeatures(TFeaturesInfo & features);

private:
  bool m_isLocked = false;
  threads::Mutex m_mutex;
  set<FeatureID> m_features;
};

} // namespace df
