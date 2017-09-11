#include "testing/testing.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/point2d.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "routing_common/car_model.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include <vector>

using namespace routing;
using namespace integration;

// The test on combinatorial explosion of number of fake edges at FeaturesRoadGraph.
// It might happen when a lot of roads intersect at one point. For example,
// http://www.openstreetmap.org/#map=19/50.73197/-1.21295
UNIT_TEST(FakeEdgesCombinatorialExplosion)
{
  classificator::Load();

  std::vector<LocalCountryFile> localFiles;
  GetAllLocalFiles(localFiles);
  TEST(!localFiles.empty(), ());

  Index index;
  for (auto const & file : localFiles)
  {
    auto const result = index.Register(file);
    TEST_EQUAL(result.second, MwmSet::RegResult::Success, ());
  }

  FeaturesRoadGraph graph(index, IRoadGraph::Mode::ObeyOnewayTag,
                          make_shared<CarModelFactory>(CountryParentNameGetterFn()));
  Junction const j(m2::PointD(MercatorBounds::FromLatLon(50.73208, -1.21279)), feature::kDefaultAltitudeMeters);
  std::vector<std::pair<routing::Edge, routing::Junction>> sourceVicinity;
  graph.FindClosestEdges(j.GetPoint(), 20 /* count */, sourceVicinity);
  // In case of the combinatorial explosion mentioned above all the memory was consumed for
  // FeaturesRoadGraph::m_fakeIngoingEdges and FeaturesRoadGraph::m_fakeOutgoingEdges fields.
  graph.AddFakeEdges(j, sourceVicinity);
}
