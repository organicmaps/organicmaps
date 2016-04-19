#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_engine.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
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
 * \brief The TurnCandidate struct contains information about possible ways from a junction.
 */
struct TurnCandidate
{
  /*!
   * angle is an angle of the turn in degrees. It means angle is 180 minus
   * an angle between the current edge and the edge of the candidate. A counterclockwise rotation.
   * The current edge is an edge which belongs the route and located before the junction.
   * angle belongs to the range [-180; 180];
   */
  double angle;
  /*!
   * node is a possible node (a possible way) from the juction.
   * May be NodeId for OSRM router or FeatureId::index for graph router.
   */
  uint32_t node;
  /*!
   * \brief highwayClass field for the road class caching. Because feature reading is a long
   * function.
   */
  ftypes::HighwayClass highwayClass;

  TurnCandidate(double a, uint32_t n, ftypes::HighwayClass c) : angle(a), node(n), highwayClass(c)
  {
  }
};

using TTurnCandidates = vector<TurnCandidate>;

/*!
 * \brief The IRoutingResultGraph interface for the routing result. Uncouple router from the
 * annotation code that describes turns. See routers for detail implementations.
 */
class IRoutingResultGraph
{
public:
  virtual vector<LoadedPathSegment> const & GetSegments() const = 0;
  virtual void GetPossibleTurns(NodeID node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const = 0;
  virtual double GetShortestPathLength() const = 0;
  virtual m2::PointD const & GetStartPoint() const = 0;
  virtual m2::PointD const & GetEndPoint() const = 0;

  virtual ~IRoutingResultGraph() {}
};

/*!
 * \brief Compute turn and time estimation structs for the abstract route result.
 * \param routingResult abstract routing result to annotate.
 * \param delegate Routing callbacks delegate.
 * \param points Storage for unpacked points of the path.
 * \param turnsDir output turns annotation storage.
 * \param times output times annotation storage.
 * \param streets output street names along the path.
 * \return routing operation result code.
 */
IRouter::ResultCode MakeTurnAnnotation(turns::IRoutingResultGraph const & result,
                                       RouterDelegate const & delegate, vector<m2::PointD> & points,
                                       Route::TTurns & turnsDir, Route::TTimes & times,
                                       Route::TStreets & streets);

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
void GetTurnDirection(IRoutingResultGraph const & result, turns::TurnInfo & turnInfo,
                      TurnItem & turn);

/*!
 * \brief Finds an UTurn that starts from current segment and returns how many segments it lasts.
 * Returns 0 if there is no UTurn.
 * Warning! currentSegment must be greater than 0.
 */
size_t CheckUTurnOnRoute(vector<LoadedPathSegment> const & segments, size_t currentSegment, TurnItem & turn);
}  // namespace routing
}  // namespace turns
