#pragma once

#include "generator/osm_element.hpp"

#include <string>

namespace generator
{
namespace osm_element
{
bool IsPoi(OsmElement const & osmElement);

bool IsBuilding(OsmElement const & osmElement);

// Has house name or house number.
bool HasHouse(OsmElement const & osmElement);

bool HasStreet(OsmElement const & osmElement);

uint64_t GetPopulation(std::string const & populationStr);
uint64_t GetPopulation(OsmElement const & osmElement);
}  // namespace osm_element
}  // namespace generator
