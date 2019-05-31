#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"

#include "indexer/feature_data.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <map>

namespace generator
{
using namespace std;

template <typename Dataset>
class EmitterBooking : public EmitterInterface
{
public:
  EmitterBooking(Dataset const & dataset, map<base::GeoObjectId, feature::FeatureBuilder> & features)
    : m_dataset(dataset), m_features(features)
  {
  }

  // EmitterInterface overrides:
  void Process(feature::FeatureBuilder & fb) override
  {
    if (m_dataset.NecessaryMatchingConditionHolds(fb))
      m_features.emplace(fb.GetMostGenericOsmId(), fb);
  }

  void GetNames(vector<string> & names) const override
  {
    names.clear();
  }

  bool Finish() override
  {
    LOG_SHORT(LINFO, ("Num of booking elements:", m_features.size()));
    return true;
  }

private:
  Dataset const & m_dataset;
  map<base::GeoObjectId, feature::FeatureBuilder> & m_features;
};
}  // namespace generator
