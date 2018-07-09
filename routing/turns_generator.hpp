#pragma once

#include "routing/loaded_path_segment.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/turns.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/segment.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "traffic/traffic_info.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

struct PathData;

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
using TGetIndexFunction = std::function<size_t(std::pair<size_t, size_t>)>;

/*!
 * \brief Index of point in TUnpackedPathSegments. |m_segmentIndex| is a zero based index in vector
 * TUnpackedPathSegments. |m_pathIndex| in a zero based index in LoadedPathSegment::m_path.
 */
struct RoutePointIndex
{
  size_t m_segmentIndex = 0;
  size_t m_pathIndex = 0;

  bool operator==(RoutePointIndex const & index) const;
};

/*!
 * \brief The TurnInfo structure is a representation of a junction.
 * It has ingoing and outgoing edges and method to check if these edges are valid.
 */
struct TurnInfo
{
  LoadedPathSegment const & m_ingoing;
  LoadedPathSegment const & m_outgoing;

  TurnInfo(LoadedPathSegment const & ingoingSegment, LoadedPathSegment const & outgoingSegment)
      : m_ingoing(ingoingSegment), m_outgoing(outgoingSegment)
  {
  }

  bool IsSegmentsValid() const;
};

/*!
 * \brief Calculates |nextIndex| which is an index of next route point at result.GetSegments().
 * |nextIndex| may be calculated in forward and backward direction.
 * \returns true if |nextIndex| is calculated and false otherwise. This method returns false
 * if looking for next index should be stopped. For example in case of the point before first,
 * after the last in the route and sometimes at the end of features and at feature intersections.
 * \note This method is asymmetric. It means that in some cases it's possible to cross
 * the border between turn segments in forward direction, but in backward direction (|forward| == false)
 * it's impossible. So when turn segment border is reached in backward direction the method
 * returns false. The reasons why crossing the border in backward direction is not implemented is
 * it's almost not used now and it's difficult to cover it with unit and integration tests.
 * As a consequence moving in backward direction it's possible to get index of the first item
 * of a segment. But it's impossible moving in forward direction.
 */
bool GetNextRoutePointIndex(IRoutingResult const & result, RoutePointIndex const & index,
                            NumMwmIds const & numMwmIds, bool forward, RoutePointIndex & nextIndex);

/*!
 * \brief Compute turn and time estimation structs for the abstract route result.
 * \param result abstract routing result to annotate.
 * \param delegate Routing callbacks delegate.
 * \param points Storage for unpacked points of the path.
 * \param turnsDir output turns annotation storage.
 * \param streets output street names along the path.
 * \param segments route segments.
 * \return routing operation result code.
 */
RouterResultCode MakeTurnAnnotation(IRoutingResult const & result, NumMwmIds const & numMwmIds,
                                       RouterDelegate const & delegate, std::vector<Junction> & points,
                                       Route::TTurns & turnsDir, Route::TStreets & streets,
                                       std::vector<Segment> & segments);

// Returns the distance in meractor units for the path of points for the range [startPointIndex, endPointIndex].
double CalculateMercatorDistanceAlongPath(uint32_t startPointIndex, uint32_t endPointIndex,
                                          std::vector<m2::PointD> const & points);

/*!
 * \brief Selects lanes which are recommended for an end user.
 */
void SelectRecommendedLanes(Route::TTurns & turnsDir);
void FixupTurns(std::vector<Junction> const & points, Route::TTurns & turnsDir);
inline size_t GetFirstSegmentPointIndex(std::pair<size_t, size_t> const & p) { return p.first; }

CarDirection InvertDirection(CarDirection dir);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows along the rightmost possible way.
 */
CarDirection RightmostDirection(double angle);
CarDirection LeftmostDirection(double angle);

/*!
 * \param angle is an angle of a turn. It belongs to a range [-180, 180].
 * \return correct direction if the route follows not along one of two outermost ways
 * or if there is only one possible way.
 */
CarDirection IntermediateDirection(double angle);

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
CarDirection GetRoundaboutDirection(bool isIngoingEdgeRoundabout, bool isOutgoingEdgeRoundabout,
                                    bool isMultiTurnJunction, bool keepTurnByHighwayClass);

/*!
 * \brief GetTurnDirection makes a primary decision about turns on the route.
 * \param outgoingSegmentIndex index of an outgoing segments in vector result.GetSegments().
 * \param turn is used for keeping the result of turn calculation.
 */
void GetTurnDirection(IRoutingResult const & result, size_t outgoingSegmentIndex,
                      NumMwmIds const & numMwmIds, TurnItem & turn);

/*!
 * \brief Finds an U-turn that starts from master segment and returns how many segments it lasts.
 * \returns an index in |segments| that has the opposite direction with master segment
 * (|segments[currentSegment - 1]|) and 0 if there is no UTurn.
 * \warning |currentSegment| must be greater than 0.
 */
size_t CheckUTurnOnRoute(IRoutingResult const & result, size_t outgoingSegmentIndex,
                         NumMwmIds const & numMwmIds, TurnItem & turn);

std::string DebugPrint(RoutePointIndex const & index);
}  // namespace routing
}  // namespace turns
