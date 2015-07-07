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

size_t const AverageFeaturesCount = 2048;
using TFeaturesInfo = buffer_vector<FeatureInfo, AverageFeaturesCount>;

class MemoryFeatureIndex : private noncopyable
{
public:
  void ReadFeaturesRequest(TFeaturesInfo & features, vector<FeatureID> & featuresToRead);
  void RemoveFeatures(TFeaturesInfo & features);

private:
  threads::Mutex m_mutex;
  set<FeatureID> m_features;
};

} // namespace df
