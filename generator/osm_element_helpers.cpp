#include "generator/osm_element_helpers.hpp"

#include <algorithm>
#include <set>
#include <string>

namespace generator
{
namespace osm_element
{
bool IsPoi(OsmElement const & osmElement)
{
  static std::set<std::string> const kPoiTypes = {"amenity", "shop", "tourism", "leisure",
                                                  "sport", "craft", "man_made", "office",
                                                  "historic", "railway", "aeroway"};
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return kPoiTypes.find(t.key) != std::end(kPoiTypes);
  });
}

bool IsBuilding(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.key == "building";
  });
}

bool HasHouse(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.key == "addr:housenumber" || t.key == "addr:housename";
  });
}

bool HasStreet(OsmElement const & osmElement)
{
  auto const & tags = osmElement.Tags();
  return std::any_of(std::cbegin(tags), std::cend(tags), [](OsmElement::Tag const & t) {
    return t.key == "addr:street";
  });
}
}  // namespace osm_element
}  // namespace generator
