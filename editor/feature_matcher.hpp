#pragma once

#include "editor/xml_feature.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "std/vector.hpp"

namespace matcher
{
/// Returns closest to the latLon node from osm or empty node if none is close enough.
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon);
/// Returns a way from osm with similar geometry or empty node if can't find such way.
pugi::xml_node GetBestOsmWayOrRelation(pugi::xml_document const & osmResponse,
                                       vector<m2::PointD> const & geometry);
/// Returns value form [-1, 1]. Negative values are used as penalty, positive as score.
/// |lhs| and |rhs| - triangulated polygons;
double ScoreTriangulatedGeometries(vector<m2::PointD> const & lhs, vector<m2::PointD> const & rhs);
}  // namespace osm
