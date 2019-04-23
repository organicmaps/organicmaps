#include "generator/streets/streets_filter.hpp"

#include "generator/streets/streets_builder.hpp"
#include "generator/osm_element_helpers.hpp"

namespace generator
{
namespace streets
{
bool StreetsFilter::IsAccepted(OsmElement const & element)
{
  return StreetsBuilder::IsStreet(element);
}

bool StreetsFilter::IsAccepted(FeatureBuilder1 const & feature)
{
  return feature.GetParams().IsValid() && IsStreet(feature);
}

// static
bool StreetsFilter::IsStreet(FeatureBuilder1 const & fb)
{
  return StreetsBuilder::IsStreet(fb);
}
}  // namespace streets
}  // namespace generator
