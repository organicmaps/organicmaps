#include "generator/osm_element_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <algorithm>
#include <set>

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

uint64_t GetPopulation(std::string const & populationStr)
{
  std::string number;
  for (auto const c : populationStr)
  {
    if (isdigit(c))
      number += c;
    else if (c == '.' || c == ',' || c == ' ')
      continue;
    else
      break;
  }

  if (number.empty())
    return 0;

  uint64_t result = 0;
  if (!strings::to_uint64(number, result))
    LOG(LWARNING, ("Failed to get population from", number, populationStr));

  return result;
}

uint64_t GetPopulation(OsmElement const & osmElement)
{
  return GetPopulation(osmElement.GetTag("population"));
}
}  // namespace osm_element
}  // namespace generator
