#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>

namespace generator
{
// This class is implementation of the FeatureProcessorInterface for coastlines.
class ProcessorCoastline : public FeatureProcessorInterface
{
public:
  explicit ProcessorCoastline(std::shared_ptr<FeatureProcessorQueue> const & queue);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & feature) override;
  void Finish() override;

private:
  std::shared_ptr<AffiliationsFeatureLayer<>> m_affiliationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
