#include "generator/translator_geo_objects.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_interface.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

namespace generator
{
namespace
{
class FilterGeoObjects : public FilterInterface
{
public:
  // FilterInterface overrides:
  bool IsAccepted(OsmElement const & element) override
  {
    return osm_element::IsBuilding(element) || osm_element::IsPoi(element);
  }

  bool IsAccepted(FeatureBuilder1 const & feature) override
  {
    return feature.GetParams().IsValid() && !feature.IsLine();
  }
};
}  // namespace

TranslatorGeoObjects::TranslatorGeoObjects(std::shared_ptr<EmitterInterface> emitter,
                                           cache::IntermediateDataReader & holder)
  : Translator(emitter, holder, std::make_shared<FeatureMakerSimple>(holder))

{
  AddFilter(std::make_shared<FilterGeoObjects>());
}
}  // namespace generator
