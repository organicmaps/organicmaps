#pragma once

#include "routing/loaded_path_segment.hpp"
#include "routing/turn_candidate.hpp"

#include "std/vector.hpp"

namespace routing
{
namespace turns
{

/*!
 * \brief The IRoutingResultGraph interface for the routing result. Uncouple router from the
 * annotation code that describes turns. See routers for detail implementations.
 */
class IRoutingResultGraph
{
public:
  virtual vector<LoadedPathSegment> const & GetSegments() const = 0;
  virtual void GetPossibleTurns(TUniversalNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const = 0;
  virtual double GetShortestPathLength() const = 0;
  virtual m2::PointD const & GetStartPoint() const = 0;
  virtual m2::PointD const & GetEndPoint() const = 0;

  virtual ~IRoutingResultGraph() = default;
};
}  // namespace routing
}  // namespace turns
