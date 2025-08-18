#pragma once

#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>
#include <string>

namespace generator
{
// This class is the implementation of FeatureProcessorInterface for countries.
class ProcessorCountry : public FeatureProcessorInterface
{
public:
  ProcessorCountry(AffiliationInterfacePtr affiliations, std::shared_ptr<FeatureProcessorQueue> queue);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & feature) override;
  void Finish() override;

private:
  AffiliationInterfacePtr m_affiliations;
  std::shared_ptr<FeatureProcessorQueue> m_queue;

  std::shared_ptr<AffiliationsFeatureLayer> m_affiliationsLayer;
  std::shared_ptr<LayerBase> m_processingChain;
  // std::shared_ptr<ComplexFeaturesMixer> m_complexFeaturesMixer;
};

}  // namespace generator
