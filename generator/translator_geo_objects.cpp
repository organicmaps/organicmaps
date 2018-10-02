#include "generator/translator_geo_objects.hpp"

#include "generator/collector_interface.hpp"
#include "generator/emitter_interface.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

namespace generator
{
TranslatorGeoObjects::TranslatorGeoObjects(std::shared_ptr<EmitterInterface> emitter,
                                           cache::IntermediateDataReader & holder)
  : TranslatorGeocoderBase(emitter, holder) {}

bool TranslatorGeoObjects::IsSuitableElement(OsmElement const * p) const
{
  return osm_element::IsBuilding(*p) || osm_element::IsPoi(*p);
}
}  // namespace generator
