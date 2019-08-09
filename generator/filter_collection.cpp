#include "generator/filter_collection.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include <algorithm>

using namespace feature;

namespace generator
{
std::shared_ptr<FilterInterface> FilterCollection::Clone() const
{
  auto p = std::make_shared<FilterCollection>();
  for (auto const & c : m_collection)
   p->Append(c->Clone());
  return p;
}

bool FilterCollection::IsAccepted(OsmElement const & element)
{
  return std::all_of(std::begin(m_collection), std::end(m_collection), [&] (auto & filter) {
    return filter->IsAccepted(element);
  });
}

bool FilterCollection::IsAccepted(FeatureBuilder const & feature)
{
  return std::all_of(std::begin(m_collection), std::end(m_collection), [&] (auto & filter) {
    return filter->IsAccepted(feature);
  });
}
}  // namespace generator
