#include "../testing/testing.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"

#include "../routing/astar_router.hpp"
#include "../routing/features_road_graph.hpp"

#include "../base/logging.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


void TestTwoPoints(uint32_t featureIdStart, uint32_t segIdStart, uint32_t featureIdFinal, uint32_t segIdFinal)
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

  vector<routing::RoadPos> startPos = {{featureIdStart, true, segIdStart}, {featureIdStart, false, segIdStart}};
  vector<routing::RoadPos> finalPos = {{featureIdFinal, true, segIdFinal}, {featureIdFinal, false, segIdFinal}};
  router.SetFinalRoadPos(finalPos);

  vector<routing::RoadPos> route;
  LOG(LINFO, ("Calculating routing..."));
  router.CalculateRoute(startPos, route);
  LOG(LINFO, ("Route length:", route.size()));
}

UNIT_TEST(PedestrianRouting_UK_Long1)
{
  TestTwoPoints(59231052, 8, 49334376, 0);
}

UNIT_TEST(PedestrianRouting_UK_Medium1)
{
  TestTwoPoints(3038057, 0, 45899679, 3);
}

UNIT_TEST(PedestrianRouting_UK_Short1)
{
  TestTwoPoints(3038057, 0, 3032688, 3);
}
