#include "../testing/testing.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"

#include "../routing/astar_router.hpp"
#include "../routing/features_road_graph.hpp"

#include "../base/logging.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

UNIT_TEST(PedestrianRouting_UK)
{
  string const kMapName = "UK_England";
  classificator::Load();
  Index index;
  routing::AStarRouter router(&index);

  m2::RectD rect;
  index.AddMap(kMapName + DATA_FILE_EXTENSION, rect);
  TEST(index.IsLoaded(kMapName), ());
  MwmSet::MwmId id = index.GetMwmIdByName(kMapName + DATA_FILE_EXTENSION);
  TEST_NOT_EQUAL(static_cast<size_t>(-1), id, ());

  router.SetRoadGraph(new routing::FeaturesRoadGraph(&index, id));

  vector<routing::RoadPos> startPos = {{59231052, true, 8}, {59231052, false, 8}};
  vector<routing::RoadPos> finalPos = {{49334376, true, 0}, {49334376, false, 0}};
  router.SetFinalRoadPos(finalPos);

  vector<routing::RoadPos> route;
  LOG(LINFO, ("Calculating routing..."));
  router.CalculateRoute(startPos, route);
  LOG(LINFO, ("Route length:", route.size()));
}
