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
                        turns::TTurnsGeom & turnsGeom,
                        my::Cancellable const & cancellable) = 0;
};

}  // namespace routing
