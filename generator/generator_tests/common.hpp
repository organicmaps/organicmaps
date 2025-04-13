#pragma once
#include "generator/osm_element.hpp"

#include <string>
#include <vector>

namespace generator_tests
{

using Tags = std::vector<std::pair<std::string, std::string>>;

OsmElement MakeOsmElement(uint64_t id, Tags const & tags, OsmElement::EntityType t);

std::string GetFileName(std::string const & filename = {});

}  // namespace generator_tests
