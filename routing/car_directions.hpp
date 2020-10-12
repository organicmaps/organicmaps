#pragma once

#include "routing/directions_engine.hpp"
#include "routing/directions_engine_helpers.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/segment.hpp"
#include "routing/turn_candidate.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point_with_altitude.hpp"

#include <map>
#include <memory>
#include <vector>

namespace routing
{
class CarDirectionsEngine : public DirectionsEngine
{
public:
  CarDirectionsEngine(DataSource const & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);

  // DirectionsEngine override:
  bool Generate(IndexRoadGraph const & graph, std::vector<geometry::PointWithAltitude> const & path,
                base::Cancellable const & cancellable, Route::TTurns & turns,
                Route::TStreets & streetNames,
                std::vector<geometry::PointWithAltitude> & routeGeometry,
                std::vector<Segment> & segments) override;
};
}  // namespace routing
