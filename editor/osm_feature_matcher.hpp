#pragma once

#include "editor/xml_feature.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "std/vector.hpp"

namespace osm
{
/// @returns closest to the latLon node from osm or empty node if none is close enough.
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon);
/// @returns a way from osm with similar geometry or empy node if can't find such way.
pugi::xml_node GetBestOsmWay(pugi::xml_document const & osmResponse,
                             vector<m2::PointD> const & geometry);
}  // namespace osm
