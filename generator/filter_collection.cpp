#include "generator/filter_collection.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <algorithm>

namespace generator
{
bool FilterCollection::IsAccepted(OsmElement const & element)
{
  return std::all_of(std::begin(m_collection), std::end(m_collection), [&] (auto & filter) {
    return filter->IsAccepted(element);
  });
}

bool FilterCollection::IsAccepted(FeatureBuilder1 const & feature)
{
  return std::all_of(std::begin(m_collection), std::end(m_collection), [&] (auto & filter) {
    return filter->IsAccepted(feature);
  });
}
}  // namespace generator
