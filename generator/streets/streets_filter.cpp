#include "generator/streets/streets_filter.hpp"

#include "generator/streets/streets_builder.hpp"
#include "generator/osm_element_helpers.hpp"

using namespace feature;

namespace generator
{
namespace streets
{
std::shared_ptr<FilterInterface> StreetsFilter::Clone() const
{
  return std::make_shared<StreetsFilter>();
}

bool StreetsFilter::IsAccepted(OsmElement const & element)
{
  return StreetsBuilder::IsStreet(element);
}

bool StreetsFilter::IsAccepted(FeatureBuilder const & feature)
{
  return feature.GetParams().IsValid() && IsStreet(feature);
}

// static
bool StreetsFilter::IsStreet(FeatureBuilder const & fb)
{
  return StreetsBuilder::IsStreet(fb);
}
}  // namespace streets
}  // namespace generator
