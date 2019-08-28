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
                           std::shared_ptr<cache::IntermediateData> const & cache,
                           feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorWorld & other) const override;

protected:
  using Translator::Translator;

  TagAdmixer m_tagAdmixer;
  TagReplacer m_tagReplacer;
};

// The TranslatorWorldWithAds class implements translator for building the world with advertising.
class TranslatorWorldWithAds : public TranslatorWorld
{
public:
  explicit TranslatorWorldWithAds(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                  std::shared_ptr<cache::IntermediateData> const & cache,
                                  feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;
  bool Save() override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorWorldWithAds & other) const override;

protected:
  using TranslatorWorld::TranslatorWorld;

  OsmTagMixer m_osmTagMixer;

private:
  // Fix warning 'hides overloaded virtual function'.
  using TranslatorWorld::MergeInto;
};
}  // namespace generator
