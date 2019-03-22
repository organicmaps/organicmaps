#include "generator/translator_world.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_planet.hpp"
#include "generator/filter_elements.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "defines.hpp"

namespace generator
{
TranslatorWorld::TranslatorWorld(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                                 feature::GenerateInfo const & info)
  : Translator(emitter, holder, std::make_shared<FeatureMaker>(holder))
  , m_tagAdmixer(info.GetIntermediateFileName("ways", ".csv"), info.GetIntermediateFileName("towns", ".csv"))
  , m_tagReplacer(GetPlatform().ResourcesDir() + REPLACED_TAGS_FILE)
  , m_osmTagMixer(GetPlatform().ResourcesDir() + MIXED_TAGS_FILE)
{
  AddFilter(std::make_shared<FilterPlanet>());
  AddFilter(std::make_shared<FilterElements>(base::JoinPath(GetPlatform().ResourcesDir(), SKIPPED_ELEMENTS_FILE)));
}

void TranslatorWorld::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_tagReplacer(element);
  m_tagAdmixer(element);
  m_osmTagMixer(element);
}
}  // namespace generator
