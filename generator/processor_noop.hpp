#pragma once

#include "generator/feature_builder.hpp"
#include "generator/processor_interface.hpp"

#include <memory>

class FeatureParams;

namespace generator
{
class ProcessorNoop : public FeatureProcessorInterface
{
public:
  // FeatureProcessorInterface overrides:
  std::shared_ptr<FeatureProcessorInterface> Clone() const override { return std::make_shared<ProcessorNoop>(); }

  void Process(feature::FeatureBuilder &) override {}
  void Finish() override {}
};
}  // namespace generator
