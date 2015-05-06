#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/route.hpp"
#include "routing/turns.hpp"

#include "std/function.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

struct PathData;
class Index;

namespace routing
{
struct RoutingMapping;

namespace turns
{
// Returns a segment index by STL-like range [s, e) of segments indices for passed node.
typedef function<size_t(pair<size_t, size_t>)> TGetIndexFunction;

OsrmMappingTypes::FtSeg GetSegment(PathData const & node, RoutingMapping const & routingMapping,
                                   TGetIndexFunction GetIndex);
vector<SingleLaneInfo> GetLanesInfo(PathData const & node,
                                                 RoutingMapping const & routingMapping,
                                                 TGetIndexFunction GetIndex, Index const & index);
/// CalculateTurnGeometry calculates geometry for all the turns. That means that for every turn
/// CalculateTurnGeometry calculates a sequence of points which will be used
/// for displaying arrows on the route.
void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir,
                           TurnsGeomT & turnsGeom);
// Selects lanes which are recommended for the end user.
void AddingActiveLaneInformation(Route::TurnsT & turnsDir);
void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir);
}
}
