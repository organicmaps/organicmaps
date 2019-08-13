#pragma once

#include "generator/processor_interface.hpp"
#include "generator/translator.hpp"

#include <memory>

namespace cache
{
class IntermediateData;
}  // namespace cache

namespace generator
{
// The TranslatorStreets class implements translator for streets.
// Every Street is either a highway or a place=square.
class TranslatorStreets : public Translator
{
public:
  explicit TranslatorStreets(std::shared_ptr<FeatureProcessorInterface> const & processor,
                             std::shared_ptr<cache::IntermediateData> const & cache);

  // TranslatorInterface overrides:
  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorStreets & other) const override;

protected:
  using Translator::Translator;
};
}  // namespace generator
