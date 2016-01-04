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

struct FeatureInfo
{
  FeatureInfo()
    : m_isOwner(false) {}

  FeatureInfo(FeatureID const & id)
    : m_id(id), m_isOwner(false) {}

  bool operator < (FeatureInfo const & other) const
  {
    if (m_id != other.m_id)
      return m_id < other.m_id;

    return m_isOwner < other.m_isOwner;
  }

  FeatureID m_id;
  bool m_isOwner;
};

// It is better for TileInfo to have size that is equal or slightly less
// than several memory pages.
size_t const kAverageFeaturesCount = 2040;
using TFeaturesInfo = buffer_vector<FeatureInfo, kAverageFeaturesCount>;

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

  void ReadFeaturesRequest(TFeaturesInfo & features, vector<FeatureID> & featuresToRead);
  void RemoveFeatures(TFeaturesInfo & features);

private:
  bool m_isLocked = false;
  threads::Mutex m_mutex;
  set<FeatureID> m_features;
};

} // namespace df
