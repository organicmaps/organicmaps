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
/// Returns a segment index by STL-like range [s, e) of segments indices for passed node.
typedef function<size_t(pair<size_t, size_t>)> TGetIndexFunction;

size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p);
OsrmMappingTypes::FtSeg GetSegment(NodeID node, RoutingMapping const & routingMapping,
                                   TGetIndexFunction GetIndex);
vector<SingleLaneInfo> GetLanesInfo(NodeID node, RoutingMapping const & routingMapping,
                                    TGetIndexFunction GetIndex, Index const & index);
/// CalculateTurnGeometry calculates geometry for all the turns. That means that for every turn
/// CalculateTurnGeometry calculates a sequence of points which will be used
/// for displaying arrows on the route.
void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir,
                           TurnsGeomT & turnsGeom);
/// Selects lanes which are recommended for an end user.
void SelectRecommendedLanes(Route::TurnsT & turnsDir);
void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir);
ftypes::HighwayClass GetOutgoingHighwayClass(NodeID node,
                                             RoutingMapping const & routingMapping,
                                             Index const & index);
TurnDirection InvertDirection(TurnDirection dir);
TurnDirection MostRightDirection(double angle);
TurnDirection MostLeftDirection(double angle);
TurnDirection IntermediateDirection(double angle);

bool KeepOnewayOutgoingTurnRoundabout(bool isRound1, bool isRound2);
/// Returns false (that means it removes the turn between ingoingClass and outgoingClass)
/// if (1) the route leads from one big road to another one; (2) the other possible turns lead to small roads;
/// and (2) turn is GoStraight or TurnSlight*.
bool KeepMultiTurnClassHighwayClass(ftypes::HighwayClass ingoingClass, ftypes::HighwayClass outgoingClass,
                                    NodeID outgoingNode, TurnDirection turn,
                                    TurnCandidatesT const & possibleTurns,
                                    RoutingMapping const & routingMapping, Index const & index);
TurnDirection RoundaboutDirection(bool isRound1, bool isRound2,
                                         bool hasMultiTurns);
} // namespace routing
} // namespace turns
