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
 * \brief The LoadedPathSegment struct is a representation of a single osrm node path.
 * It unpacks and stores information about path and road type flags.
 * Postprocessing must read information from the structure and does not initiate disk readings.
 */
struct LoadedPathSegment
{
  vector<m2::PointD> m_path;
  ftypes::HighwayClass m_highwayClass;
  bool m_onRoundabout;
  bool m_isLink;
  EdgeWeight m_weight;
  string m_name;
  NodeID m_nodeId;
  vector<SingleLaneInfo> m_lanes;

  // General constructor.
  LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                    RawPathData const & osrmPathSegment);
  // Special constructor for side nodes. Splits OSRM node by information from the FeatureGraphNode.
  LoadedPathSegment(RoutingMapping & mapping, Index const & index,
                    RawPathData const & osrmPathSegment, FeatureGraphNode const & startGraphNode,
                    FeatureGraphNode const & endGraphNode, bool isStartNode, bool isEndNode);

private:
  // Load information about road, that described as the sequence of FtSegs and start/end indexes in
  // in it. For the side case, it has information about start/end graph nodes.
  void LoadPathGeometry(buffer_vector<OsrmMappingTypes::FtSeg, 8> const & buffer, size_t startIndex,
                        size_t endIndex, Index const & index, RoutingMapping & mapping,
                        FeatureGraphNode const & startGraphNode,
                        FeatureGraphNode const & endGraphNode, bool isStartNode, bool isEndNode);
};

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
void GetTurnDirection(Index const & index, RoutingMapping & mapping, turns::TurnInfo & turnInfo,
                      TurnItem & turn);

/*!
 * \brief Finds an UTurn that starts from current segment and returns how many segments it lasts.
 * Returns 0 if there is no UTurn.
 * Warning! currentSegment must be greater than 0.
 */
size_t CheckUTurnOnRoute(vector<LoadedPathSegment> const & segments, size_t currentSegment, TurnItem & turn);
}  // namespace routing
}  // namespace turns
