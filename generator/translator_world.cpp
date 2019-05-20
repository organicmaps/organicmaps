#include "generator/translator_world.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_planet.hpp"
#include "generator/filter_elements.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/node_mixer.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

namespace generator
{
TranslatorWorld::TranslatorWorld(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & cache,
                                 feature::GenerateInfo const & info)
  : Translator(emitter, cache, std::make_shared<FeatureMaker>(cache))
  , m_tagAdmixer(info.GetIntermediateFileName("ways", ".csv"), info.GetIntermediateFileName("towns", ".csv"))
  , m_tagReplacer(GetPlatform().ResourcesDir() + REPLACED_TAGS_FILE)
{
  AddFilter(std::make_shared<FilterPlanet>());
  AddFilter(std::make_shared<FilterElements>(base::JoinPath(GetPlatform().ResourcesDir(), SKIPPED_ELEMENTS_FILE)));
}

void TranslatorWorld::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_tagReplacer(element);
  m_tagAdmixer(element);
}

TranslatorWorldWithAds::TranslatorWorldWithAds(std::shared_ptr<EmitterInterface> emitter,
                                               cache::IntermediateDataReader & cache,
                                               feature::GenerateInfo const & info)
  : TranslatorWorld(emitter, cache, info)
  , m_osmTagMixer(base::JoinPath(GetPlatform().ResourcesDir(), MIXED_TAGS_FILE)) {}

void TranslatorWorldWithAds::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_osmTagMixer(element);
  TranslatorWorld::Preprocess(element);
}

bool TranslatorWorldWithAds::Finish()
{
  MixFakeNodes(GetPlatform().ResourcesDir() + MIXED_NODES_FILE,
               std::bind(&TranslatorWorldWithAds::Emit, this, std::placeholders::_1));
  return TranslatorWorld::Finish();
}
}  // namespace generator
