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

  virtual void Generate(RoadGraphBase const & graph, std::vector<Junction> const & path,
                        my::Cancellable const & cancellable, Route::TTimes & times,
                        Route::TTurns & turns, Route::TStreets & streetNames,
                        std::vector<Junction> & routeGeometry, std::vector<Segment> & segments) = 0;

protected:
  /// \brief constructs route based on |graph| and |path|. Fills |routeEdges| with the route.
  /// \returns false in case of any errors while reconstruction, if reconstruction process
  /// was cancelled and in case of extremely short paths of 0 or 1 point. Returns true otherwise.
  bool ReconstructPath(RoadGraphBase const & graph, std::vector<Junction> const & path,
                       std::vector<Edge> & routeEdges, my::Cancellable const & cancellable) const;
};
}  // namespace routing
