#pragma once

#include "routing/road_graph.hpp"
#include "routing/route.hpp"

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
                        vector<Junction> & routeGeometry, my::Cancellable const & cancellable) = 0;

protected:
  /// \brief constructs route based on |graph| and |path|. Fills |routeEdges| with the route.
  /// \returns false in case of any errors while reconstruction, if reconstruction process
  /// was cancelled and in case of extremely short paths of 0 or 1 point. Returns true otherwise.
  bool ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                       vector<Edge> & routeEdges, my::Cancellable const & cancellable) const;

  void CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                      Route::TTimes & times) const;
};

void ReconstructRoute(IDirectionsEngine * engine, IRoadGraph const & graph,
                      my::Cancellable const & cancellable, vector<Junction> & path, Route & route);
}  // namespace routing
