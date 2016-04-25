#pragma once

#include "routing/directions_engine.hpp"

namespace routing
{

class BicycleDirectionsEngine : public IDirectionsEngine
{
public:
  BicycleDirectionsEngine();

  // IDirectionsEngine override:
  void Generate(IRoadGraph const & graph, vector<Junction> const & path,
                Route::TTimes & times,
                Route::TTurns & turnsDir,
                my::Cancellable const & cancellable) override;
};
}  // namespace routing
