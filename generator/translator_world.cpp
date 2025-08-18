#include "generator/translator_world.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_collection.hpp"
#include "generator/filter_elements.hpp"
#include "generator/filter_planet.hpp"
#include "generator/filter_roads.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
// #include "generator/node_mixer.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <algorithm>
#include <string>

#include "defines.hpp"

namespace generator
{
TranslatorWorld::TranslatorWorld(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                 std::shared_ptr<cache::IntermediateData> const & cache,
                                 feature::GenerateInfo const & info)
  : Translator(processor, cache, std::make_shared<FeatureMaker>(cache->GetCache()))
  , m_tagAdmixer(std::make_shared<TagAdmixer>(info.GetIntermediateFileName("ways", ".csv"),
                                              info.GetIntermediateFileName(TOWNS_FILE)))
  , m_tagReplacer(std::make_shared<TagReplacer>(base::JoinPath(GetPlatform().ResourcesDir(), REPLACED_TAGS_FILE)))
{
  /// @todo This option is not used, but may be useful in future?
  //  if (needMixTags)
  //  {
  //    m_osmTagMixer = std::make_shared<OsmTagMixer>(
  //        base::JoinPath(GetPlatform().ResourcesDir(), MIXED_TAGS_FILE));
  //  }

  auto filters = std::make_shared<FilterCollection>();
  filters->Append(std::make_shared<FilterPlanet>());
  filters->Append(std::make_shared<FilterRoads>());
  filters->Append(
      std::make_shared<FilterElements>(base::JoinPath(GetPlatform().ResourcesDir(), SKIPPED_ELEMENTS_FILE)));
  SetFilter(filters);
}

std::shared_ptr<TranslatorInterface> TranslatorWorld::Clone() const
{
  auto copy = Translator::CloneBase<TranslatorWorld>();
  copy->m_tagAdmixer = m_tagAdmixer;
  copy->m_tagReplacer = m_tagReplacer;
  copy->m_osmTagMixer = m_osmTagMixer;
  return copy;
}

void TranslatorWorld::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_tagReplacer->Process(element);
  m_tagAdmixer->Process(element);
  if (m_osmTagMixer)
    m_osmTagMixer->Process(element);
}

void TranslatorWorld::MergeInto(TranslatorWorld & other) const
{
  MergeIntoBase(other);
}
}  // namespace generator
