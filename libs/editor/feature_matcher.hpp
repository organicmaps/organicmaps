#pragma once

#include "editor/xml_feature.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace matcher
{
/// Returns a node from OSM closest to the latLon, or an empty node if none is close enough.
pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const & latLon);
/// Returns a way from osm with similar geometry or empty node if can't find such way.
/// Throws NotAPolygon exception when |geometry| is not convertible to a polygon.
pugi::xml_node GetBestOsmWayOrRelation(pugi::xml_document const & osmResponse,
                                       std::vector<m2::PointD> const & geometry);
/// Returns value form [-1, 1]. Negative values are used as penalty, positive as score.
/// |lhs| and |rhs| - triangulated polygons.
/// Throws NotAPolygon exception when either lhs or rhs is not convertible to a polygon.
double ScoreTriangulatedGeometries(std::vector<m2::PointD> const & lhs,
                                   std::vector<m2::PointD> const & rhs);
/// Deprecated, use ScoreTriangulatedGeometries instead. In the older versions of the editor, the
/// edits.xml file didn't contain the necessary geometry information, so we cannot restore the
/// original geometry of a particular feature and thus can't use the new algo that is dependent on
/// correct feature geometry to calculate scores.
// TODO(a): To remove it when version 8.0.x is no longer supported.
double ScoreTriangulatedGeometriesByPoints(std::vector<m2::PointD> const & lhs,
                                           std::vector<m2::PointD> const & rhs);
}  // namespace osm
