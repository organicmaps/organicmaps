#include "generator/translator_region.hpp"

#include "generator/collector_interface.hpp"
#include "generator/emitter_interface.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include <algorithm>
#include <set>
#include <string>

namespace generator
{
TranslatorRegion::TranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                                   cache::IntermediateDataReader & holder,
                                   std::shared_ptr<CollectorInterface> collector)
  : TranslatorGeocoderBase(emitter, holder)
{
  AddCollector(collector);
}

bool TranslatorRegion::IsSuitableElement(OsmElement const * p) const
{
  static std::set<std::string> const places = {"city", "town", "village", "suburb", "neighbourhood",
                                               "hamlet", "locality", "isolated_dwelling"};

  for (auto const & t : p->Tags())
  {
    if (t.key == "place" && places.find(t.value) != places.end())
      return true;

    if (t.key == "boundary" && t.value == "administrative")
      return true;
  }

  return false;
}
}  // namespace generator
