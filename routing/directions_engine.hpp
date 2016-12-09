#pragma once

#include "routing/road_graph.hpp"
#include "routing/road_point.hpp"
#include "routing/route.hpp"

#include "traffic/traffic_info.hpp"

#include "base/cancellable.hpp"

#include "std/vector.hpp"

namespace routing
{
class IDirectionsEngine
{
public:
  virtual ~IDirectionsEngine() = default;

  virtual void Generate(IRoadGraph const & graph, vector<Junction> const & path,
                        Route::TTimes & times, Route::TTurns & turns,
                        vector<Junction> & routeGeometry,
                        vector<traffic::TrafficInfo::RoadSegmentId> & routeSegs,
                        my::Cancellable const & cancellable) = 0;

protected:
  /// \brief constructs route based on |graph| and |path|. Fills |routeEdges| with the route.
  /// \returns false in case of any errors while reconstruction, if reconstruction process
  /// was cancelled and in case of extremely short paths of 0 or 1 point. Returns true otherwise.
  bool ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                       vector<Edge> & routeEdges, my::Cancellable const & cancellable) const;

  void CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                      Route::TTimes & times) const;
};
}  // namespace routing
