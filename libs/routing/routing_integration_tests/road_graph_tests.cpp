#include "testing/testing.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/mwm_set.hpp"

#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"
#include "routing_common/car_model.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "geometry/point_with_altitude.hpp"

#include <memory>
#include <utility>
#include <vector>

using namespace routing;
using namespace integration;

// The test on combinatorial explosion of number of fake edges at FeaturesRoadGraph.
// It might happen when a lot of roads intersect at one point. For example,
// https://www.openstreetmap.org/#map=19/50.73197/-1.21295
UNIT_TEST(FakeEdgesCombinatorialExplosion)
{
  classificator::Load();

  std::vector<LocalCountryFile> localFiles;
  GetAllLocalFiles(localFiles);
  TEST(!localFiles.empty(), ());

  FrozenDataSource dataSource;
  for (auto const & file : localFiles)
    dataSource.Register(file);

  MwmDataSource routingSource(dataSource, nullptr /* numMwmIDs */);
  FeaturesRoadGraph graph(routingSource, IRoadGraph::Mode::ObeyOnewayTag,
                          std::make_shared<CarModelFactory>(CountryParentNameGetterFn()));
  geometry::PointWithAltitude const j(m2::PointD(mercator::FromLatLon(50.73208, -1.21279)),
                                      geometry::kDefaultAltitudeMeters);
  std::vector<std::pair<routing::Edge, geometry::PointWithAltitude>> sourceVicinity;
  graph.FindClosestEdges(mercator::RectByCenterXYAndSizeInMeters(j.GetPoint(), FeaturesRoadGraph::kClosestEdgesRadiusM),
                         20 /* count */, sourceVicinity);
  // In case of the combinatorial explosion mentioned above all the memory was consumed for
  // FeaturesRoadGraph::m_fakeIngoingEdges and FeaturesRoadGraph::m_fakeOutgoingEdges fields.
  graph.AddFakeEdges(j, sourceVicinity);
}
