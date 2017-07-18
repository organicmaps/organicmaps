#pragma once

#include "routing/road_graph.hpp"
#include "routing/road_point.hpp"
#include "routing/route.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_info.hpp"

#include "base/cancellable.hpp"

#include <vector>

namespace routing
{
class IDirectionsEngine
{
public:
  virtual ~IDirectionsEngine() = default;

  // @TODO(bykoianko) When fields |m_turns|, |m_times|, |m_streets| and |m_traffic|
  // are removed from class Route the method Generate() should fill
  // vector<RouteSegment> instead of corresponding arguments.
  /// \brief Generates all args which are passed by reference.
  /// \param path is points of the route. It should not be empty.
  /// \returns true if fields passed by reference are filled correctly and false otherwise.
  virtual bool Generate(RoadGraphBase const & graph, vector<Junction> const & path,
                        my::Cancellable const & cancellable, Route::TTurns & turns,
                        Route::TStreets & streetNames, vector<Junction> & routeGeometry,
                        vector<Segment> & segments) = 0;

protected:
  /// \brief constructs route based on |graph| and |path|. Fills |routeEdges| with the route.
  /// \returns false in case of any errors while reconstruction, if reconstruction process
  /// was cancelled and in case of extremely short paths of 0 or 1 point. Returns true otherwise.
  bool ReconstructPath(RoadGraphBase const & graph, std::vector<Junction> const & path,
                       std::vector<Edge> & routeEdges, my::Cancellable const & cancellable) const;
};
}  // namespace routing
