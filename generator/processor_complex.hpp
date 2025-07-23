#pragma once

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
// This class is the implementation of FeatureProcessorInterface for complexes.
class ProcessorComplex : public FeatureProcessorInterface
{
public:
  explicit ProcessorComplex(std::shared_ptr<FeatureProcessorQueue> const & queue, std::string const & bordersPath,
                            bool haveBordersForWholeWorld);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & feature) override;
  void Finish() override;

private:
  std::string m_bordersPath;
  std::shared_ptr<AffiliationsFeatureLayer<>> m_affiliationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
  bool m_haveBordersForWholeWorld;
};
}  // namespace generator
