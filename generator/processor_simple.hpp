#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>
#include <string>

namespace generator
{
// ProcessorSimple class is a simple processor. It does not filter objects.
class ProcessorSimple : public FeatureProcessorInterface
{
public:
  // |name| is bucket name. For example it may be "World", "geo_objects", "regions" etc.
  explicit ProcessorSimple(std::shared_ptr<FeatureProcessorQueue> const & queue, std::string const & name);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & fb) override;
  void Finish() override;

  std::string GetFilename() const { return m_name; }

private:
  std::string m_name;
  std::shared_ptr<AffiliationsFeatureLayer<feature::serialization_policy::MinSize>> m_affiliationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
