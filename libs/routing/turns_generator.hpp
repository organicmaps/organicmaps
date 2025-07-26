#pragma once

#include "routing/routing_result_graph.hpp"
#include "routing/routing_settings.hpp"
#include "routing/turn_candidate.hpp"
#include "routing/turns.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point_with_altitude.hpp"

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

struct RoutePointIndex;
struct TurnInfo;

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
bool GetNextRoutePointIndex(IRoutingResult const & result, RoutePointIndex const & index, NumMwmIds const & numMwmIds,
                            bool const forward, RoutePointIndex & nextIndex);

inline size_t GetFirstSegmentPointIndex(std::pair<size_t, size_t> const & p)
{
  return p.first;
}

bool GetTurnInfo(IRoutingResult const & result, size_t const outgoingSegmentIndex,
                 RoutingSettings const & vehicleSettings, TurnInfo & turnInfo);

/// \returns angle, wchis is calculated using several backward and forward segments
/// from junction to consider smooth turns and remove noise.
double CalcTurnAngle(IRoutingResult const & result, size_t const outgoingSegmentIndex, NumMwmIds const & numMwmIds,
                     RoutingSettings const & vehicleSettings);

void RemoveUTurnCandidate(TurnInfo const & turnInfo, NumMwmIds const & numMwmIds,
                          std::vector<TurnCandidate> & turnCandidates);

/// \returns true if there is exactly 1 turn in |turnCandidates| with angle less then
/// |kMaxForwardAngleCandidates|.
bool HasSingleForwardTurn(TurnCandidates const & turnCandidates, float maxForwardAngleCandidates);

// It's possible that |firstOutgoingSeg| is not contained in |turnCandidates|.
// It may happened if |firstOutgoingSeg| and candidates in |turnCandidates| are from different mwms.
// Let's identify it in turnCandidates by angle and update according turnCandidate.
void CorrectCandidatesSegmentByOutgoing(TurnInfo const & turnInfo, Segment const & firstOutgoingSeg,
                                        TurnCandidates & nodes);

/*!
 * \brief Returns ingoing point or outgoing point for turns.
 * These points belong to the route but they often are not neighbor of junction point.
 * To calculate the resulting point the function implements the following steps:
 * - going from junction point along route path according to the direction which is set in GetPointIndex().
 * - until one of following conditions is fulfilled:
 *   - more than |maxPointsCount| points are passed (returns the maxPointsCount-th point);
 *   - the length of passed parts of segment exceeds maxDistMeters;
 *     (returns the next point after the event)
 *   - an important bifurcation point is reached in case of outgoing point is looked up (forward == true).
 * \param result information about the route. |result.GetSegments()| is composed of LoadedPathSegment.
 * Each LoadedPathSegment is composed of several Segments. The sequence of Segments belongs to
 * single feature and does not split by other features.
 * \param outgoingSegmentIndex index in |segments|. Junction point noticed above is the first point
 * of |outgoingSegmentIndex| segment in |result.GetSegments()|.
 * \param maxPointsCount maximum number between the returned point and junction point.
 * \param maxDistMeters maximum distance between the returned point and junction point.
 * \param forward is direction of moving along the route to calculate the returned point.
 * If forward == true the direction is to route finish. If forward == false the direction is to route start.
 * \return an ingoing or outgoing point for a turn calculation.
 */
m2::PointD GetPointForTurn(IRoutingResult const & result, size_t outgoingSegmentIndex, NumMwmIds const & numMwmIds,
                           size_t const maxPointsCount, double const maxDistMeters, bool const forward);

}  // namespace turns
}  // namespace routing
