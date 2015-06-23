#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_engine.hpp"
#include "routing/route.hpp"
#include "routing/turns.hpp"

#include "std/function.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

struct PathData;
class Index;

namespace ftypes
{
enum class HighwayClass;
}

namespace routing
{
struct RoutingMapping;

namespace turns
{
/*!
 * \brief Returns a segment index by STL-like range [s, e) of segments indices for the passed node.
 */
typedef function<size_t(pair<size_t, size_t>)> TGetIndexFunction;

/*!
 * \brief The TurnInfo struct is an accumulator for all junction information.
 * During a junction (a turn) analysis a different subsets of fields in the structure
 * may be calculated. The main purpose of TurnInfo is to prevent recalculation the same fields.
 * The idea is if a field is required to check whether a field has been calculated.
 * If yes, just use it. If not, the field should be calculated, kept in the structure
 * and used.
 */
struct TurnInfo
{
  RoutingMapping & m_routeMapping;

  NodeID m_ingoingNodeID;
  OsrmMappingTypes::FtSeg m_ingoingSegment;
  ftypes::HighwayClass m_ingoingHighwayClass;

  NodeID m_outgoingNodeID;
  OsrmMappingTypes::FtSeg m_outgoingSegment;
  ftypes::HighwayClass m_outgoingHighwayClass;

  TurnInfo(RoutingMapping & routeMapping, NodeID ingoingNodeID, NodeID outgoingNodeID);

  bool IsSegmentsValid() const;
};

size_t GetLastSegmentPointIndex(pair<size_t, size_t> const & p);
vector<SingleLaneInfo> GetLanesInfo(NodeID node, RoutingMapping const & routingMapping,
                                    TGetIndexFunction GetIndex, Index const & index);

/*!
 * \brief Returns geometry for all the turns. It means that for every turn CalculateTurnGeometry
 * calculates a sequence of points.
 */
void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TTurns const & turnsDir,
                           TTurnsGeom & turnsGeom);
/*!
 * \brief Selects lanes which are recommended for an end user.
 */
void SelectRecommendedLanes(Route::TTurns & turnsDir);
void FixupTurns(vector<m2::PointD> const & points, Route::TTurns & turnsDir);
inline size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p) { return p.first; }

TurnDirection InvertDirection(TurnDirection dir);
TurnDirection RightmostDirection(double angle);
TurnDirection LeftmostDirection(double angle);
TurnDirection IntermediateDirection(double angle);

/*!
 * \return Returns true if the route enters a roundabout.
 * That means isIngoingEdgeRoundabout is false and isOutgoingEdgeRoundabout is true.
 */
bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout);
/*!
 * \return Returns a turn instruction if an ingoing edge or (and) outgoing edge belongs to a roundabout.
 */
TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isMultiTurnJunction);
/*!
 * \brief GetTurnDirection makes a primary decision about turns on the route.
 * \param turnInfo is used for cashing some information while turn calculation.
 * \param turn is used for keeping the result of turn calculation.
 */
void GetTurnDirection(Index const & index, turns::TurnInfo & turnInfo, TurnItem & turn);

}  // namespace routing
}  // namespace turns
