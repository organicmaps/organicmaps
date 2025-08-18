#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/checkpoints.hpp"
#include "routing/regions_decl.hpp"
#include "routing/router_delegate.hpp"

#include "base/thread.hpp"

#include <unordered_set>
#include <utility>
#include <vector>

namespace routing
{
class IndexGraphStarter;

// Encapsulates routing thread for generating all the mwms through which passes the route between
// |checkpoints|.
class RegionsRouter : public threads::IRoutine
{
public:
  RegionsRouter(CountryFileGetterFn const & countryFileGetter, std::shared_ptr<NumMwmIds> numMwmIds,
                DataSource & dataSource, RouterDelegate const & delegate, Checkpoints const & checkpoints);

  void Do() override;

  std::unordered_set<std::string> const & GetMwmNames() const;

private:
  template <typename Vertex, typename Edge, typename Weight>
  RouterResultCode ConvertResult(typename AStarAlgorithm<Vertex, Edge, Weight>::Result result) const;

  RouterResultCode CalculateSubrouteNoLeapsMode(IndexGraphStarter & starter, std::vector<Segment> & subroute,
                                                m2::PointD const & startCheckpoint,
                                                m2::PointD const & finishCheckpoint);

  // Gets checkpoint with |index| from |m_checkpoints| and returns its location and mwm number.
  std::pair<m2::PointD, std::string> GetCheckpointRegion(size_t index) const;

  CountryFileGetterFn const m_countryFileGetterFn;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  DataSource & m_dataSource;
  Checkpoints const m_checkpoints;
  std::unordered_set<std::string> m_mwmNames;

  RouterDelegate const & m_delegate;
};
}  // namespace routing
