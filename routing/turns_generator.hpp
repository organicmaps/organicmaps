#pragma once

#include "routing/loaded_path_segment.hpp"
#include "routing/routing_result_graph.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/turns.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

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
 * \brief Compute turn and time estimation structs for the abstract route result.
 * \param routingResult abstract routing result to annotate.
 * \param delegate Routing callbacks delegate.
 * \param points Storage for unpacked points of the path.
 * \param turnsDir output turns annotation storage.
 * \param streets output street names along the path.
 * \param traffic road traffic information.
 * \return routing operation result code.
 */
IRouter::ResultCode MakeTurnAnnotation(turns::IRoutingResult const & result,
                                       RouterDelegate const & delegate, vector<Junction> & points,
                                       Route::TTurns & turnsDir, Route::TStreets & streets,
                                       vector<Segment> & segments);

/*!
 * \brief The TurnInfo struct is a representation of a junction.
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

// Returns the distance in meractor units for the path of points for the range [startPointIndex, endPointIndex].
double CalculateMercatorDistanceAlongPath(uint32_t startPointIndex, uint32_t endPointIndex,
                                          vector<m2::PointD> const & points);

/*!
 * \brief Selects lanes which are recommended for an end user.
 */
void SelectRecommendedLanes(Route::TTurns & turnsDir);
void FixupTurns(vector<Junction> const & points, Route::TTurns & turnsDir);
inline size_t GetFirstSegmentPointIndex(pair<size_t, size_t> const & p) { return p.first; }

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
 * \param turnInfo is used for cashing some information while turn calculation.
 * \param turn is used for keeping the result of turn calculation.
 */
void GetTurnDirection(IRoutingResult const & result, turns::TurnInfo & turnInfo, TurnItem & turn);

/*!
 * \brief Finds an U-turn that starts from master segment and returns how many segments it lasts.
 * \returns an index in |segments| that has the opposite direction with master segment
 * (|segments[currentSegment - 1]|) and 0 if there is no UTurn.
 * \warning |currentSegment| must be greater than 0.
 */
size_t CheckUTurnOnRoute(TUnpackedPathSegments const & segments,
                         size_t currentSegment, TurnItem & turn);
}  // namespace routing
}  // namespace turns
