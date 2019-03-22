#include "generator/translator_country.hpp"

#include "generator/camera_node_processor.hpp"
#include "generator/collector_addresses.hpp"
#include "generator/collector_interface.hpp"
#include "generator/collector_tag.hpp"
#include "generator/feature_maker.hpp"
#include "generator/filter_elements.hpp"
#include "generator/filter_planet.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/maxspeeds_collector.hpp"
#include "generator/metalines_builder.hpp"
#include "generator/restriction_writer.hpp"
#include "generator/road_access_generator.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/assert.hpp"

#include <cctype>
#include <cstdint>
#include <string>

#include "defines.hpp"

namespace generator
{
namespace
{
class RelationCollector
{
public:
  explicit RelationCollector(CollectorCollection & collectors) : m_collectors(collectors) {}

  template <class Reader>
  base::ControlFlow operator()(uint64_t id, Reader & reader)
  {
    RelationElement element;
    CHECK(reader.Read(id, element), (id));
    m_collectors.CollectRelation(element);
    return base::ControlFlow::Continue;
  }

private:
  CollectorInterface & m_collectors;
};

// https://www.wikidata.org/wiki/Wikidata:Identifiers
bool WikiDataValidator(std::string const & tagValue)
{
  if (tagValue.size() < 2)
    return false;

  size_t pos = 0;
  // Only items are are needed.
  if (tagValue[pos++] != 'Q')
    return false;

  while (pos != tagValue.size())
  {
    if (!std::isdigit(tagValue[pos++]))
      return false;
  }

  return true;
}
}  // namespace

TranslatorCountry::TranslatorCountry(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                                     feature::GenerateInfo const & info)
  : Translator(emitter, holder, std::make_shared<FeatureMaker>(holder))
  , m_tagAdmixer(info.GetIntermediateFileName("ways", ".csv"), info.GetIntermediateFileName("towns", ".csv"))
  , m_tagReplacer(base::JoinPath(GetPlatform().ResourcesDir(), REPLACED_TAGS_FILE))
  , m_osmTagMixer(base::JoinPath(GetPlatform().ResourcesDir(), MIXED_TAGS_FILE))
{
  AddFilter(std::make_shared<FilterPlanet>());
  AddFilter(std::make_shared<FilterElements>(base::JoinPath(GetPlatform().ResourcesDir(), SKIPPED_ELEMENTS_FILE)));

  AddCollector(std::make_shared<CollectorTag>(info.m_idToWikidataFilename, "wikidata" /* tagKey */,
                                              WikiDataValidator, true /* ignoreIfNotOpen */));
  AddCollector(std::make_shared<feature::MetalinesBuilder>(info.GetIntermediateFileName(METALINES_FILENAME)));

  // These are the four collector that collect additional information for the future building of routing section.
  AddCollector(std::make_shared<MaxspeedsCollector>(info.GetIntermediateFileName(MAXSPEEDS_FILENAME)));
  AddCollector(std::make_shared<routing::RestrictionWriter>(info.GetIntermediateFileName(RESTRICTIONS_FILENAME)));
  AddCollector(std::make_shared<routing::RoadAccessWriter>(info.GetIntermediateFileName(ROAD_ACCESS_FILENAME)));
  AddCollector(std::make_shared<routing::CameraNodeProcessor>(info.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME),
                                                              info.GetIntermediateFileName(CAMERAS_NODES_TO_WAYS_FILE),
                                                              info.GetIntermediateFileName(CAMERAS_MAXSPEED_FILE)));
  if (info.m_genAddresses)
    AddCollector(std::make_shared<CollectorAddresses>(info.GetAddressesFileName()));
}

void TranslatorCountry::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_tagReplacer(element);
  m_tagAdmixer(element);
  m_osmTagMixer(element);
  CollectFromRelations(element);
}

void TranslatorCountry::CollectFromRelations(OsmElement const & element)
{
  RelationCollector collector(m_collectors);
  if (element.IsNode())
    m_holder.ForEachRelationByNodeCached(element.id, collector);
  else if (element.IsWay())
    m_holder.ForEachRelationByWayCached(element.id, collector);
}
}  // namespace generator
