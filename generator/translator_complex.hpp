#pragma once

#include "generator/filter_interface.hpp"
#include "generator/hierarchy.hpp"
#include "generator/processor_interface.hpp"
#include "generator/tag_admixer.hpp"
#include "generator/translator.hpp"

#include <memory>
#include <string>

namespace cache
{
class IntermediateData;
}  // namespace cache

namespace generator
{
// The TranslatorComplex class implements translator for building complexes.
class TranslatorComplex : public Translator
{
public:
  explicit TranslatorComplex(std::shared_ptr<FeatureProcessorInterface> const & processor,
                             std::shared_ptr<cache::IntermediateData> const & cache,
                             feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorComplex & other) const override;

protected:
  using Translator::Translator;

  std::shared_ptr<TagReplacer> m_tagReplacer;
};
}  // namespace generator
