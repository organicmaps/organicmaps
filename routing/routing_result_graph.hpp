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
  /// \returns information about all route segments.
  virtual TUnpackedPathSegments const & GetSegments() const = 0;
  /// \brief for number of a |node|, point of the node (|junctionPoint|) and for a point
  /// just before the node (|ingoingPoint|) it fills
  /// * |ingoingCount| - number of incomming ways to |junctionPoint|. (|junctionPoint| >= 1)
  /// * |outgoingTurns| - vector of ways to leave |junctionPoint|.
  virtual void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                TTurnCandidates & outgoingTurns) const = 0;
  /// \returns route length.
  virtual double GetPathLength() const = 0;
  /// \returns route start point.
  virtual m2::PointD const & GetStartPoint() const = 0;
  /// \returns route finish point.
  virtual m2::PointD const & GetEndPoint() const = 0;

  virtual ~IRoutingResultGraph() = default;
};
}  // namespace routing
}  // namespace turns
