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

UNIT_TEST(PedestrianRouting_UK_Medium1) { TestTwoPoints(3038057, 0, 45899679, 3); }

UNIT_TEST(PedestrianRouting_UK_Short1) { TestTwoPoints(3038057, 0, 3032688, 3); }
