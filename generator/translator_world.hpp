#pragma once

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
// The TranslatorWorld class implements translator for building the world.
class TranslatorWorld : public Translator
{
public:
  explicit TranslatorWorld(std::shared_ptr<FeatureProcessorInterface> const & processor,
                           std::shared_ptr<cache::IntermediateData> const & cache, feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  IMPLEMENT_TRANSLATOR_IFACE(TranslatorWorld);
  void MergeInto(TranslatorWorld & other) const;

protected:
  using Translator::Translator;

  std::shared_ptr<TagAdmixer> m_tagAdmixer;
  std::shared_ptr<TagReplacer> m_tagReplacer;
  std::shared_ptr<OsmTagMixer> m_osmTagMixer;
};
}  // namespace generator
