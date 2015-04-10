#include "../testing/testing.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"

#include "../routing/astar_router.hpp"
#include "../routing/features_road_graph.hpp"

#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/timer.hpp"

#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

pair<m2::PointD, m2::PointD> GetPointsAroundSeg(Index & index, MwmSet::MwmId id, uint32_t featureId,
                                                uint32_t segId)
{
  FeatureType ft;
  Index::FeaturesLoaderGuard loader(index, id);
  loader.GetFeature(featureId, ft);
  ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
  CHECK_LESS(segId + 1, ft.GetPointsCount(),
             ("Wrong segment id:", segId, "for a feature with", ft.GetPointsCount(), "points"));
  return make_pair(ft.GetPoint(segId), ft.GetPoint(segId + 1));
}

void TestTwoPoints(uint32_t featureIdStart, uint32_t segIdStart, uint32_t featureIdFinal,
                   uint32_t segIdFinal)
{
  string const kMapName = "UK_England";
  classificator::Load();
  Index index;
  routing::AStarRouter router(&index);

  UNUSED_VALUE(index.RegisterMap(kMapName + DATA_FILE_EXTENSION));
  TEST(index.IsLoaded(kMapName), ());
  MwmSet::MwmId id = index.GetMwmIdByName(kMapName + DATA_FILE_EXTENSION);
  TEST_NOT_EQUAL(static_cast<size_t>(-1), id, ());

  router.SetRoadGraph(make_unique<routing::FeaturesRoadGraph>(&index, id));

  pair<m2::PointD, m2::PointD> startBounds =
      GetPointsAroundSeg(index, id, featureIdStart, segIdStart);

  vector<routing::RoadPos> startPos = {{featureIdStart, true, segIdStart, startBounds.second},
                                       {featureIdStart, false, segIdStart, startBounds.first}};

  pair<m2::PointD, m2::PointD> finalBounds =
      GetPointsAroundSeg(index, id, featureIdStart, segIdStart);
  vector<routing::RoadPos> finalPos = {{featureIdFinal, true, segIdFinal, finalBounds.second},
                                       {featureIdFinal, false, segIdFinal, finalBounds.first}};

  vector<routing::RoadPos> route;

  my::Timer timer;
  LOG(LINFO, ("Calculating routing..."));
  routing::IRouter::ResultCode resultCode = router.CalculateRouteM2M(startPos, finalPos, route);
  CHECK_EQUAL(routing::IRouter::NoError, resultCode, ());
  LOG(LINFO, ("Route length:", route.size()));
  LOG(LINFO, ("Elapsed:", timer.ElapsedSeconds(), "seconds"));
}

UNIT_TEST(PedestrianRouting_UK_Long1) { TestTwoPoints(59231052, 8, 49334376, 0); }

UNIT_TEST(PedestrianRouting_UK_Long2) { TestTwoPoints(2909201, 1, 86420951, 1); }

UNIT_TEST(PedestrianRouting_UK_Long3) { TestTwoPoints(46185185, 1, 44584579, 4); }

UNIT_TEST(PedestrianRouting_UK_Long4) { TestTwoPoints(42085288, 2, 52107406, 6); }

UNIT_TEST(PedestrianRouting_UK_Long5) { TestTwoPoints(25581618, 31, 24932741, 3); }

UNIT_TEST(PedestrianRouting_UK_Long6) { TestTwoPoints(87984202, 3, 84929880, 0); }

UNIT_TEST(PedestrianRouting_UK_Medium1) { TestTwoPoints(3038057, 0, 45899679, 3); }

UNIT_TEST(PedestrianRouting_UK_Medium2) { TestTwoPoints(42689385, 1, 14350838, 5); }

UNIT_TEST(PedestrianRouting_UK_Medium3) { TestTwoPoints(43922917, 7, 44173940, 1); }

UNIT_TEST(PedestrianRouting_UK_Medium4) { TestTwoPoints(45414223, 1, 46093762, 2); }

UNIT_TEST(PedestrianRouting_UK_Medium5) { TestTwoPoints(45862427, 4, 4449317, 7); }

UNIT_TEST(PedestrianRouting_UK_Short1) { TestTwoPoints(3038057, 0, 3032688, 3); }

UNIT_TEST(PedestrianRouting_UK_Short2) { TestTwoPoints(2947484, 2, 44889742, 0); }

UNIT_TEST(PedestrianRouting_UK_Short3) { TestTwoPoints(2931545, 0, 2969395, 0); }

