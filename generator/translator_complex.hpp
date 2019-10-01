#pragma once

#include "generator/processor_interface.hpp"
#include "generator/translator.hpp"

#include <memory>
#include <string>

namespace cache
{
class IntermediateData;
}  // namespace cache

namespace generator
{
// The TranslatorComplex class implements translator for building map objects complex.
class TranslatorComplex : public Translator
{
public:
  explicit TranslatorComplex(std::shared_ptr<FeatureProcessorInterface> const & processor,
                             std::shared_ptr<cache::IntermediateData> const & cache);

  // TranslatorInterface overrides:
  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorComplex & other) const override;

protected:
  using Translator::Translator;
};
}  // namespace generator
