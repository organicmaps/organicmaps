#pragma once

#include "routing/road_graph.hpp"
#include "routing/route.hpp"

#include "base/cancellable.hpp"

namespace routing
{

class IDirectionsEngine
{
public:
  virtual ~IDirectionsEngine() = default;

  virtual void Generate(IRoadGraph const & graph, vector<Junction> const & path,
                        Route::TTimes & times,
                        Route::TTurns & turnsDir,
                        my::Cancellable const & cancellable) = 0;
protected:
  bool ReconstructPath(IRoadGraph const & graph, vector<Junction> const & path,
                       vector<Edge> & routeEdges,
                       my::Cancellable const & cancellable) const;

  void CalculateTimes(IRoadGraph const & graph, vector<Junction> const & path,
                      Route::TTimes & times) const;
};

}  // namespace routing
