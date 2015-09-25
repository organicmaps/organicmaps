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
using TGetIndexFunction = function<size_t(pair<size_t, size_t>)>;

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
  bool m_isIngoingEdgeRoundabout;

  NodeID m_outgoingNodeID;
  OsrmMappingTypes::FtSeg m_outgoingSegment;
  ftypes::HighwayClass m_outgoingHighwayClass;
  bool m_isOutgoingEdgeRoundabout;

  TurnInfo(RoutingMapping & routeMapping, NodeID ingoingNodeID, NodeID outgoingNodeID);

  bool IsSegmentsValid() const;
};

size_t GetLastSegmentPointIndex(pair<size_t, size_t> const & p);
vector<SingleLaneInfo> GetLanesInfo(NodeID node, RoutingMapping const & routingMapping,
                                    TGetIndexFunction GetIndex, Index const & index);

// Returns the distance in meractor units for the path of points for the range [startPointIndex, endPointIndex].
double CalculateMercatorDistanceAlongPath(uint32_t startPointIndex, uint32_t endPointIndex,
                                          vector<m2::PointD> const & points);

/*!
 * \brief Selects lanes which are recommended for an end user.
 */
void SelectRecommendedLanes(Route::TTurns & turnsDir);
void FixupTurns(vector<m2::PointD> const & points, Route::TTurns & turnsDir);
inline size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p) { return p.first; }

TurnDirection InvertDirection(TurnDirection dir);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows along the rightmost possible way.
 */
TurnDirection RightmostDirection(double angle);
TurnDirection LeftmostDirection(double angle);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows not along one of two outermost ways
 * or if there is only one possible way.
 */
TurnDirection IntermediateDirection(double angle);

/*!
 * \return Returns true if the route enters a roundabout.
 * That means isIngoingEdgeRoundabout is false and isOutgoingEdgeRoundabout is true.
 */
bool CheckRoundaboutEntrance(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout);

/*!
 * \return Returns true if the route leaves a roundabout.
 * That means isIngoingEdgeRoundabout is true and isOutgoingEdgeRoundabout is false.
 */
bool CheckRoundaboutExit(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout);

/*!
 * \brief Calculates a turn instruction if the ingoing edge or (and) the outgoing edge belongs to a
 * roundabout.
 * \return Returns one of the following results:
 * - TurnDirection::EnterRoundAbout if the ingoing edge does not belong to a roundabout
 *   and the outgoing edge belongs to a roundabout.
 * - TurnDirection::StayOnRoundAbout if the ingoing edge and the outgoing edge belong to a
 * roundabout
 *   and there is a reasonalbe way to leave the junction besides the outgoing edge.
 *   This function does not return TurnDirection::StayOnRoundAbout for small ways to leave the
 * roundabout.
 * - TurnDirection::NoTurn if the ingoing edge and the outgoing edge belong to a roundabout
 *   (a) and there is a single way (outgoing edge) to leave the junction.
 *   (b) and there is a way(s) besides outgoing edge to leave the junction (the roundabout)
 *       but it is (they are) relevantly small.
 */
TurnDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                     bool isMultiTurnJunction, bool keepTurnByHighwayClass);



/*!
 * \brief GetTurnDirection makes a primary decision about turns on the route.
 * \param turnInfo is used for cashing some information while turn calculation.
 * \param turn is used for keeping the result of turn calculation.
 */
void GetTurnDirection(Index const & index, turns::TurnInfo & turnInfo, TurnItem & turn);

}  // namespace routing
}  // namespace turns
