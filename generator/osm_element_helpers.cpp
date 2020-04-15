#include "generator/osm_element_helpers.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <algorithm>
#include <set>

namespace generator
{
namespace osm_element
{
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
