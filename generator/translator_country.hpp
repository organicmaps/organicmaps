#pragma once

#include "generator/affiliation.hpp"
#include "generator/processor_interface.hpp"
#include "generator/tag_admixer.hpp"
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
// The TranslatorArea class implements translator for building countries.
class TranslatorCountry : public Translator
{
public:
  TranslatorCountry(std::shared_ptr<FeatureProcessorInterface> const & processor,
                    std::shared_ptr<cache::IntermediateData> const & cache, feature::GenerateInfo const & info,
                    AffiliationInterfacePtr affiliation);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  IMPLEMENT_TRANSLATOR_IFACE(TranslatorCountry);
  void MergeInto(TranslatorCountry & other) const;

protected:
  using Translator::Translator;

  void CollectFromRelations(OsmElement const & element);

  std::shared_ptr<TagAdmixer> m_tagAdmixer;
  std::shared_ptr<TagReplacer> m_tagReplacer;
  std::shared_ptr<OsmTagMixer> m_osmTagMixer;
};
}  // namespace generator
