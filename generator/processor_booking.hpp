#pragma once

#include "generator/processor_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"

#include "indexer/feature_data.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <map>

namespace generator
{
using namespace std;

template <typename Dataset>
class ProcessorBooking : public FeatureProcessorInterface
{
public:
  ProcessorBooking(Dataset const & dataset, map<base::GeoObjectId, feature::FeatureBuilder> & features)
    : m_dataset(dataset), m_features(features) {}

  // FeatureProcessorInterface overrides:
  virtual std::shared_ptr<FeatureProcessorInterface> Clone() const
  {
    CHECK(false, ());
  }


  void Process(feature::FeatureBuilder & fb) override
  {
    if (m_dataset.NecessaryMatchingConditionHolds(fb))
      m_features.emplace(fb.GetMostGenericOsmId(), fb);
  }

  void Flush() override {}

  bool Finish() override
  {
    LOG_SHORT(LINFO, ("Num of booking elements:", m_features.size()));
    return true;
  }

  void Merge(FeatureProcessorInterface const &) override
  {
    CHECK(false, ());
  }

private:
  Dataset const & m_dataset;
  map<base::GeoObjectId, feature::FeatureBuilder> & m_features;
};
}  // namespace generator
