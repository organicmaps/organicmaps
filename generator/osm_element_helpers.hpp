#pragma once

#include "generator/osm_element.hpp"

#include <string>
#include <vector>

namespace generator
{
namespace osm_element
{
uint64_t GetPopulation(std::string const & str);
uint64_t GetPopulation(OsmElement const & elem);

/// @return All admin_centre and label role Nodes.
std::vector<uint64_t> GetPlaceNodeFromMembers(OsmElement const & elem);

/// @return 0 If no valid admin_level tag.
uint8_t GetAdminLevel(OsmElement const & elem);
}  // namespace osm_element
}  // namespace generator
