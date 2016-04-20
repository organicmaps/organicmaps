#pragma once

#include "routing/turns.hpp"

#include "std/vector.hpp"

namespace ftypes
{
enum class HighwayClass;
}

namespace routing
{
namespace turns
{
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
  TNodeId node;
  /*!
   * \brief highwayClass field for the road class caching. Because feature reading is a long
   * function.
   */
  ftypes::HighwayClass highwayClass;

  TurnCandidate(double a, TNodeId n, ftypes::HighwayClass c) : angle(a), node(n), highwayClass(c)
  {
  }
};

using TTurnCandidates = vector<TurnCandidate>;
}  // namespace routing
}  // namespace turns
