#include "generator/osm_element_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <algorithm>
#include <set>
#include <string>

namespace generator
{
namespace osm_element
{
bool IsPoi(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return ftypes::IsPoiChecker::kPoiTypes.find(t.m_key) != std::end(ftypes::IsPoiChecker::kPoiTypes);
  });
}

bool IsBuilding(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.m_key == "building";
  });
}

bool HasHouse(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.m_key == "addr:housenumber" || t.m_key == "addr:housename";
  });
}

bool HasStreet(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.m_key == "addr:street";
  });
}
}  // namespace osm_element
}  // namespace generator
