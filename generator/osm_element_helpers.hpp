#pragma once

#include "generator/osm_element.hpp"

#include <string>

namespace generator
{
namespace osm_element
{
uint64_t GetPopulation(std::string const & populationStr);
uint64_t GetPopulation(OsmElement const & osmElement);
}  // namespace osm_element
}  // namespace generator
