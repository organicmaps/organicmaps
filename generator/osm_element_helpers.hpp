#pragma once

#include "generator/osm_element.hpp"

namespace generator
{
namespace osm_element
{
bool IsPoi(OsmElement const & osmElement);

bool IsBuilding(OsmElement const & osmElement);

// Has house name or house number.
bool HasHouse(OsmElement const & osmElement);

bool HasStreet(OsmElement const & osmElement);
}  // namespace osm_element
}  // namespace generator
