#pragma once

#include "generator/processor_interface.hpp"
#include "generator/translator.hpp"

#include <memory>

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace cache
{
class IntermediateData;
}  // namespace cache

namespace generator
{
// The TranslatorCoastline class implements translator for building coastlines.
class TranslatorCoastline : public Translator
{
public:
  explicit TranslatorCoastline(std::shared_ptr<FeatureProcessorInterface> const & processor,
                               std::shared_ptr<cache::IntermediateData> const & cache);

  // TranslatorInterface overrides:
  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorCoastline & other) const override;

protected:
  using Translator::Translator;
};
}  // namespace generator
