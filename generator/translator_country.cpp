#include "generator/translator_country.hpp"

#include "generator/addresses_collector.hpp"
#include "generator/collector_boundary_postcode.hpp"
#include "generator/collector_camera.hpp"
#include "generator/collector_collection.hpp"
#include "generator/collector_interface.hpp"
#include "generator/collector_mini_roundabout.hpp"
#include "generator/collector_routing_city_boundaries.hpp"
#include "generator/collector_tag.hpp"
#include "generator/cross_mwm_osm_ways_collector.hpp"
#include "generator/feature_maker.hpp"
#include "generator/filter_collection.hpp"
#include "generator/filter_elements.hpp"
#include "generator/filter_planet.hpp"
#include "generator/filter_roads.hpp"
#include "generator/generate_info.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/maxspeeds_collector.hpp"
#include "generator/metalines_builder.hpp"
// #include "generator/node_mixer.hpp"
#include "generator/restriction_writer.hpp"
#include "generator/road_access_generator.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include "defines.hpp"

namespace generator
{
namespace
{
class RelationCollector
{
public:
  explicit RelationCollector(std::shared_ptr<CollectorInterface> const & collectors) : m_collectors(collectors) {}

  template <class Reader>
  base::ControlFlow operator()(uint64_t id, Reader & reader)
  {
    RelationElement element;
    CHECK(reader.Read(id, element), (id));
    m_collectors->CollectRelation(element);
    return base::ControlFlow::Continue;
  }

private:
  std::shared_ptr<CollectorInterface> m_collectors;
};

// https://www.wikidata.org/wiki/Wikidata:Identifiers
bool WikiDataValidator(std::string const & tagValue)
{
  if (tagValue.size() < 2)
    return false;

  size_t pos = 0;
  // Only Q-items are needed.
  if (tagValue[pos++] != 'Q')
    return false;

  while (pos != tagValue.size())
    if (!std::isdigit(tagValue[pos++]))
      return false;

  return true;
}
}  // namespace

TranslatorCountry::TranslatorCountry(std::shared_ptr<FeatureProcessorInterface> const & processor,
                                     std::shared_ptr<cache::IntermediateData> const & cache,
                                     feature::GenerateInfo const & info, AffiliationInterfacePtr affiliation)
  : Translator(processor, cache, std::make_shared<FeatureMaker>(cache->GetCache()))
  /// @todo Looks like ways.csv was in some MM proprietary generator routine?!
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

  auto collectors = std::make_shared<CollectorCollection>();
  collectors->Append(std::make_shared<feature::MetalinesBuilder>(info.GetIntermediateFileName(METALINES_FILENAME)));
  collectors->Append(std::make_shared<BoundaryPostcodeCollector>(
      info.GetIntermediateFileName(BOUNDARY_POSTCODES_FILENAME), cache->GetCache()));

  // Collectors for gathering of additional information for the future building of routing section.
  collectors->Append(std::make_shared<RoutingCityBoundariesCollector>(
      info.GetIntermediateFileName(CITY_BOUNDARIES_COLLECTOR_FILENAME), cache->GetCache()));
  collectors->Append(std::make_shared<MaxspeedsCollector>(info.GetIntermediateFileName(MAXSPEEDS_FILENAME)));
  collectors->Append(std::make_shared<routing_builder::RestrictionWriter>(
      info.GetIntermediateFileName(RESTRICTIONS_FILENAME), cache->GetCache()));
  collectors->Append(std::make_shared<routing_builder::RoadAccessCollector>(
      info.GetIntermediateFileName(ROAD_ACCESS_FILENAME), cache->GetCache()));
  collectors->Append(std::make_shared<routing_builder::CameraCollector>(
      info.GetIntermediateFileName(CAMERAS_TO_WAYS_FILENAME), cache->GetCache()));
  collectors->Append(std::make_shared<MiniRoundaboutCollector>(info.GetIntermediateFileName(MINI_ROUNDABOUTS_FILENAME),
                                                               cache->GetCache()));
  collectors->Append(std::make_shared<AddressesCollector>(info.GetIntermediateFileName(ADDR_INTERPOL_FILENAME)));

  if (affiliation)
    collectors->Append(std::make_shared<CrossMwmOsmWaysCollector>(info.m_intermediateDir, affiliation));

  if (!info.m_idToWikidataFilename.empty())
  {
    collectors->Append(
        std::make_shared<CollectorTag>(info.m_idToWikidataFilename, "wikidata" /* tagKey */, WikiDataValidator));
  }

  SetCollector(collectors);
}

std::shared_ptr<TranslatorInterface> TranslatorCountry::Clone() const
{
  auto copy = Translator::CloneBase<TranslatorCountry>();
  copy->m_tagAdmixer = m_tagAdmixer;
  copy->m_tagReplacer = m_tagReplacer;
  copy->m_osmTagMixer = m_osmTagMixer;
  return copy;
}

void TranslatorCountry::Preprocess(OsmElement & element)
{
  // Here we can add new tags to the elements!
  m_tagReplacer->Process(element);
  m_tagAdmixer->Process(element);
  if (m_osmTagMixer)
    m_osmTagMixer->Process(element);
  CollectFromRelations(element);
}

void TranslatorCountry::MergeInto(TranslatorCountry & other) const
{
  MergeIntoBase(other);
}

void TranslatorCountry::CollectFromRelations(OsmElement const & element)
{
  auto const & cache = m_cache->GetCache();
  cache::IntermediateDataReaderInterface::ForEachRelationFn wrapper = RelationCollector(m_collector);
  if (element.IsNode())
    cache->ForEachRelationByNodeCached(element.m_id, wrapper);
  else if (element.IsWay())
    cache->ForEachRelationByWayCached(element.m_id, wrapper);
}
}  // namespace generator
