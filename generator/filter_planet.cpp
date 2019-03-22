#include "generator/filter_planet.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace generator
{
bool FilterPlanet::IsAccepted(OsmElement const & element)
{
  if (element.IsRelation())
    return element.HasAnyTag({{"type", "multipolygon"}, {"type", "boundary"}});
  if (element.IsNode())
    return !element.m_tags.empty();

  return true;
}

bool FilterPlanet::IsAccepted(FeatureBuilder1 const & feature)
{
  auto const & params = feature.GetParams();
  return params.IsValid();
}
}  // namespace generator
