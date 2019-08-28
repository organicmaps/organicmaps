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
// The TranslatorArea class implements translator for building countries.
class TranslatorCountry : public Translator
{
public:  
  explicit TranslatorCountry(std::shared_ptr<FeatureProcessorInterface> const & processor,
                             std::shared_ptr<cache::IntermediateData> const & cache,
                             feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorCountry & other) const override;

protected:
  using Translator::Translator;

  void CollectFromRelations(OsmElement const & element);

  TagAdmixer m_tagAdmixer;
  TagReplacer m_tagReplacer;
};

// The TranslatorCountryWithAds class implements translator for building countries with advertising.
class TranslatorCountryWithAds : public TranslatorCountry
{
public:
  explicit TranslatorCountryWithAds(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                    std::shared_ptr<cache::IntermediateData> const & cache,
                                    feature::GenerateInfo const & info);

  // TranslatorInterface overrides:
  void Preprocess(OsmElement & element) override;
  bool Save() override;

  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Merge(TranslatorInterface const & other) override;
  void MergeInto(TranslatorCountryWithAds & other) const override;

protected:
  using TranslatorCountry::TranslatorCountry;

  OsmTagMixer m_osmTagMixer;

private:
  // Fix warning 'hides overloaded virtual function'.
  using TranslatorCountry::MergeInto;
};
}  // namespace generator
