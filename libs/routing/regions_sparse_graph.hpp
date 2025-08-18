#pragma once

#include "routing/cross_border_graph.hpp"
#include "routing/fake_ending.hpp"
#include "routing/latlon_with_altitude.hpp"
#include "routing/regions_decl.hpp"
#include "routing/segment.hpp"

#include "routing/base/small_list.hpp"

#include <optional>

namespace routing
{

// Graph used in IndexGraphStarter for building routes through the most important roads which are
// extracted from World.mwm. These roads are presented as pairs of points between regions.
class RegionsSparseGraph
{
public:
  RegionsSparseGraph(CountryFileGetterFn const & countryFileGetter, std::shared_ptr<NumMwmIds> numMwmIds,
                     DataSource & dataSource);

  // Loads data from mwm section.
  void LoadRegionsSparseGraph();

  std::optional<FakeEnding> GetFakeEnding(m2::PointD const & point) const;

  using EdgeListT = SmallList<SegmentEdge>;
  void GetEdgeList(Segment const & segment, bool isOutgoing, EdgeListT & edges, ms::LatLon const & prevSegFront) const;

  routing::LatLonWithAltitude const & GetJunction(Segment const & segment, bool front) const;

  RouteWeight CalcSegmentWeight(Segment const & segment) const;

private:
  CrossBorderSegment const & GetDataById(RegionSegmentId const & id) const;

  CrossBorderGraph m_graph;

  CountryFileGetterFn const m_countryFileGetterFn;
  std::shared_ptr<NumMwmIds> m_numMwmIds = nullptr;
  DataSource & m_dataSource;
};
}  // namespace routing
