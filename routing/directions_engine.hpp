#pragma once

#include "routing/index_road_graph.hpp"
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

  // @TODO(bykoianko) Method Generate() should fill
  // vector<RouteSegment> instead of corresponding arguments.
  /// \brief Generates all args which are passed by reference.
  /// \param path is points of the route. It should not be empty.
  /// \returns true if fields passed by reference are filled correctly and false otherwise.
  virtual bool Generate(IndexRoadGraph const & graph, std::vector<Junction> const & path,
                        base::Cancellable const & cancellable, Route::TTurns & turns,
                        Route::TStreets & streetNames, std::vector<Junction> & routeGeometry,
                        std::vector<Segment> & segments) = 0;
  virtual void Clear() = 0;
};
}  // namespace routing
