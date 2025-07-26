#include "generator/filter_collection.hpp"

#include "base/stl_helpers.hpp"

namespace generator
{
std::shared_ptr<FilterInterface> FilterCollection::Clone() const
{
  auto p = std::make_shared<FilterCollection>();
  for (auto const & c : m_collection)
    p->Append(c->Clone());
  return p;
}

bool FilterCollection::IsAccepted(OsmElement const & element) const
{
  return base::AllOf(m_collection, [&](auto const & filter) { return filter->IsAccepted(element); });
}

bool FilterCollection::IsAccepted(feature::FeatureBuilder const & feature) const
{
  return base::AllOf(m_collection, [&](auto const & filter) { return filter->IsAccepted(feature); });
}
}  // namespace generator
