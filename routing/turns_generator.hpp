#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/route.hpp"
#include "routing/turns.hpp"

#include "indexer/ftypes_matcher.hpp"

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
// Returns a segment index by STL-like range [s, e) of segments indices for the passed node.
typedef function<size_t(pair<size_t, size_t>)> TGetIndexFunction;

size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p);
OsrmMappingTypes::FtSeg GetSegment(NodeID node, RoutingMapping const & routingMapping,
                                   TGetIndexFunction GetIndex);
vector<SingleLaneInfo> GetLanesInfo(NodeID node, RoutingMapping const & routingMapping,
                                    TGetIndexFunction GetIndex, Index const & index);
// Returns geometry for all the turns. That means that for every turn CalculateTurnGeometry calculates
// a sequence of points.
void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir,
                           TurnsGeomT & turnsGeom);
// Selects lanes which are recommended for an end user.
void SelectRecommendedLanes(Route::TurnsT & turnsDir);
void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir);
ftypes::HighwayClass GetOutgoingHighwayClass(NodeID node, RoutingMapping const & routingMapping,
                                             Index const & index);
TurnDirection InvertDirection(TurnDirection dir);
TurnDirection MostRightDirection(double angle);
TurnDirection MostLeftDirection(double angle);
TurnDirection IntermediateDirection(double angle);

// Returns true if the route enters a roundabout.
// That means isIngoingEdgeRoundabout is false and isOutgoingEdgeRoundabout is true.
bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout);
// Returns a turn instruction if an ingoing edge or (and) outgoing edge belongs to a roundabout.
TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isJunctionOfSeveralTurns);

// Returns false when
// * the route leads from one big road to another one;
// * and the other possible turns lead to small roads;
// * and the turn is GoStraight or TurnSlight*.
bool HighwayClassFilter(ftypes::HighwayClass ingoingClass, ftypes::HighwayClass outgoingClass,
                        NodeID outgoingNode, TurnDirection turn,
                        TTurnCandidates const & possibleTurns,
                        RoutingMapping const & routingMapping, Index const & index);
}  // namespace routing
}  // namespace turns
