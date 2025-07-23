#pragma once

#include "routing/loaded_path_segment.hpp"
#include "routing/turn_candidate.hpp"

#include "geometry/point_with_altitude.hpp"

namespace routing
{
namespace turns
{
/*!
 * \brief The IRoutingResult interface for the routing result. Uncouple router from the
 * annotation code that describes turns. See routers for detail implementations.
 */
class IRoutingResult
{
public:
  /// \returns information about all route segments.
  virtual TUnpackedPathSegments const & GetSegments() const = 0;
  /// \brief For a |segmentRange|, |junctionPoint| and |ingoingPoint| (point before the |junctionPoint|)
  /// this method computes number of ingoing ways to |junctionPoint| and fills |outgoingTurns|.
  /// \note This method should not be called for |segmentRange| of fake edges.
  /// So method |segmentRange.IsClear()| should return false.
  virtual void GetPossibleTurns(SegmentRange const & segmentRange, m2::PointD const & junctionPoint,
                                size_t & ingoingCount, TurnCandidates & outgoingTurns) const = 0;
  virtual double GetPathLength() const = 0;
  virtual geometry::PointWithAltitude GetStartPoint() const = 0;
  virtual geometry::PointWithAltitude GetEndPoint() const = 0;

  virtual ~IRoutingResult() = default;
};
}  // namespace turns
}  // namespace routing
