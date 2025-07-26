#include "generator/translator_complex.hpp"

#include "generator/collector_building_parts.hpp"
#include "generator/feature_maker.hpp"
#include "generator/filter_collection.hpp"
#include "generator/filter_complex.hpp"
#include "generator/filter_elements.hpp"
#include "generator/filter_planet.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

using namespace feature;

namespace generator
{
TranslatorComplex::TranslatorComplex(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                     std::shared_ptr<cache::IntermediateData> const & cache,
                                     feature::GenerateInfo const & info)
  : Translator(processor, cache, std::make_shared<FeatureMaker>(cache->GetCache()))
  , m_tagReplacer(std::make_shared<TagReplacer>(base::JoinPath(GetPlatform().ResourcesDir(), REPLACED_TAGS_FILE)))
{
  auto filters = std::make_shared<FilterCollection>();
  filters->Append(std::make_shared<FilterPlanet>());
  filters->Append(std::make_shared<FilterComplex>());
  filters->Append(
      std::make_shared<FilterElements>(base::JoinPath(GetPlatform().ResourcesDir(), SKIPPED_ELEMENTS_FILE)));
  SetFilter(filters);

  SetCollector(std::make_shared<BuildingPartsCollector>(info.GetIntermediateFileName(BUILDING_PARTS_MAPPING_FILE),
                                                        cache->GetCache()));
}

void TranslatorComplex::Preprocess(OsmElement & element)
{
  m_tagReplacer->Process(element);
}

std::shared_ptr<TranslatorInterface> TranslatorComplex::Clone() const
{
  auto copy = Translator::CloneBase<TranslatorComplex>();
  copy->m_tagReplacer = m_tagReplacer;
  return copy;
}

void TranslatorComplex::Merge(TranslatorInterface const & other)
{
  other.MergeInto(*this);
}

void TranslatorComplex::MergeInto(TranslatorComplex & other) const
{
  MergeIntoBase(other);
}
}  // namespace generator
