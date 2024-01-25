#include "generator/filter_planet.hpp"

namespace generator
{
std::shared_ptr<FilterInterface> FilterPlanet::Clone() const
{
  return std::make_shared<FilterPlanet>();
}

bool FilterPlanet::IsAccepted(OsmElement const & element) const
{
  if (element.IsRelation())
  {
    auto const type = element.GetTag("type");
    return (type == "multipolygon" || type == "boundary");
  }
  if (element.IsNode())
    return !element.m_tags.empty();

  return true;
}

bool FilterPlanet::IsAccepted(feature::FeatureBuilder const & feature) const
{
  auto const & params = feature.GetParams();
  return params.IsValid();
}
}  // namespace generator
