#pragma once
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <string>

namespace generator_tests
{
using Tags = std::vector<std::pair<std::string, std::string>>;

OsmElement MakeOsmElement(uint64_t id, Tags const & tags, OsmElement::EntityType t);

std::string GetFileName(std::string const & filename = std::string());

bool MakeFakeBordersFile(std::string const & intemediatePath, std::string const & filename);
}  // namespace generator_tests
