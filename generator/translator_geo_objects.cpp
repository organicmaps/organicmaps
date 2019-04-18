#include "generator/translator_geo_objects.hpp"

#include "generator/feature_maker.hpp"
#include "generator/filter_interface.hpp"
#include "generator/geo_objects/geo_objects_filter.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

namespace generator
{
TranslatorGeoObjects::TranslatorGeoObjects(std::shared_ptr<EmitterInterface> emitter,
                                           cache::IntermediateDataReader & cache)
  : Translator(emitter, cache, std::make_shared<FeatureMakerSimple>(cache))

{
  AddFilter(std::make_shared<geo_objects::GeoObjectsFilter>());
}
}  // namespace generator
