#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/processor_interface.hpp"

#include "indexer/feature_data.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <map>
#include <memory>

namespace generator
{
template <typename Dataset>
class ProcessorBooking : public FeatureProcessorInterface
{
public:
  ProcessorBooking(Dataset const & dataset, std::map<base::GeoObjectId, feature::FeatureBuilder> & features)
    : m_dataset(dataset)
    , m_features(features)
  {}

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override
  {
    CHECK(false, ());
    return {};
  }

  void Process(feature::FeatureBuilder & fb) override
  {
    if (m_dataset.NecessaryMatchingConditionHolds(fb))
      m_features.emplace(fb.GetMostGenericOsmId(), fb);
  }

  void Finish() override {}

private:
  Dataset const & m_dataset;
  std::map<base::GeoObjectId, feature::FeatureBuilder> & m_features;
};
}  // namespace generator
