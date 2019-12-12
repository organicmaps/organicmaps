#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/processor_interface.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
// This class is the implementation of FeatureProcessorInterface for countries.
class ProcessorCountry : public FeatureProcessorInterface
{
public:
  explicit ProcessorCountry(std::shared_ptr<FeatureProcessorQueue> const & queue,
                            std::string const & bordersPath, bool haveBordersForWholeWorld,
                            std::shared_ptr<ComplexFeaturesMixer> const & complexFeaturesMixer);

  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override;

  void Process(feature::FeatureBuilder & feature) override;
  void Finish() override;

private:
  std::string m_bordersPath;
  std::shared_ptr<AffiliationsFeatureLayer<>> m_affiliationsLayer;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::shared_ptr<LayerBase> m_processingChain;
  std::shared_ptr<ComplexFeaturesMixer> m_complexFeaturesMixer;
  bool m_haveBordersForWholeWorld;
};
}  // namespace generator
