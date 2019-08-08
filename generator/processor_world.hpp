#pragma once

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>
#include <string>

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
// This class is implementation of FeatureProcessorInterface for the world.
class ProcessorWorld : public FeatureProcessorInterface
{
public:
  explicit ProcessorWorld(std::shared_ptr<FeatureProcessorQueue> const & queue,
                          std::string const & popularityFilename);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & feature) override;
  void Finish() override;

  void Merge(FeatureProcessorInterface const & other) override;
  void MergeInto(ProcessorWorld & other) const override;

private:
  std::string m_popularityFilename;
  std::shared_ptr<AffilationsFeatureLayer<>> m_affilationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
