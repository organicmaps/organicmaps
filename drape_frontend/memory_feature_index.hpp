#pragma once

#include "tile_info.hpp"

#include "../base/mutex.hpp"

#include "../std/set.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

namespace df
{
  class MemoryFeatureIndex
  {
  public:
    MemoryFeatureIndex();

    void ReadFeaturesRequest(const vector<FeatureInfo> & features, vector<size_t> & indexes);
    void RemoveFeatures(const vector<FeatureInfo> & features);

  private:
    threads::Mutex m_mutex;
    set<FeatureID> m_features;
  };
}
