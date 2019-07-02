#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>
#include <string>
#include <vector>

namespace generator
{
// ProcessorSimpleWriter class is a simple emitter. It does not filter objects.
class ProcessorSimple : public FeatureProcessorInterface
{
public:
  explicit ProcessorSimple(std::shared_ptr<FeatureProcessorQueue> const & queue,
                           std::string const & filename);

  // EmitterInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & fb) override;
  void Flush() override;
  bool Finish() override;

  void Merge(FeatureProcessorInterface const & other) override;
  void MergeInto(ProcessorSimple & other) const override;

  std::string GetFilename() const { return m_filename; }

private:
  std::string m_filename;
  std::shared_ptr<AffilationsFeatureLayer<feature::serialization_policy::MinSize>> m_affilationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
};
}  // namespace generator
