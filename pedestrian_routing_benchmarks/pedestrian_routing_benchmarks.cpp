#include "testing/testing.hpp"

#include "indexer/index.hpp"
#include "indexer/classificator_loader.hpp"

#include "routing/astar_router.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/route.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/timer.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace
{

string const MAP_NAME = "UK_England";
string const MAP_FILE = MAP_NAME + DATA_FILE_EXTENSION;

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

void TestTwoPoints(routing::IRouter & router, m2::PointD const & startPos, m2::PointD const & finalPos)
{
  LOG(LINFO, ("Calculating routing..."));
  routing::Route route("");
  my::Timer timer;
  routing::IRouter::ResultCode const resultCode = router.CalculateRoute(startPos, m2::PointD::Zero() /* startDirection */,
                                                                        finalPos, route);
  double const elapsedSec = timer.ElapsedSeconds();
  TEST_EQUAL(routing::IRouter::NoError, resultCode, ());
  LOG(LINFO, ("Route polyline size:", route.GetPoly().GetSize()));
  LOG(LINFO, ("Route distance, meters:", route.GetDistance()));
  LOG(LINFO, ("Elapsed, seconds:", elapsedSec));
}

void TestTwoPoints(Index & index, m2::PointD const & startPos, m2::PointD const & finalPos)
{
  routing::AStarRouter router([](m2::PointD const & /* point */) { return MAP_FILE; },
                              &index);
  TestTwoPoints(router, startPos, finalPos);
}

void TestTwoPoints(uint32_t featureIdStart, uint32_t segIdStart, uint32_t featureIdFinal,
                   uint32_t segIdFinal)
{
  classificator::Load();

  Index index;
  UNUSED_VALUE(index.RegisterMap(MAP_FILE));
  TEST(index.IsLoaded(MAP_NAME), ());
  MwmSet::MwmId const id = index.GetMwmIdByFileName(MAP_FILE);
  TEST(id.IsAlive(), ());

  pair<m2::PointD, m2::PointD> const startBounds = GetPointsAroundSeg(index, id, featureIdStart, segIdStart);
  pair<m2::PointD, m2::PointD> const finalBounds = GetPointsAroundSeg(index, id, featureIdFinal, segIdFinal);

  m2::PointD const startPos = startBounds.first;
  m2::PointD const finalPos = finalBounds.first;

  TestTwoPoints(index, startPos, finalPos);
}

void TestTwoPoints(m2::PointD const & startPos, m2::PointD const & finalPos)
{
  classificator::Load();

  Index index;
  UNUSED_VALUE(index.RegisterMap(MAP_FILE));
  TEST(index.IsLoaded(MAP_NAME), ());

  TestTwoPoints(index, startPos, finalPos);
}

}  // namespace

// Tests on features

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

// Tests on points

UNIT_TEST(PedestrianRouting_UK_Test1)
{
  TestTwoPoints(m2::PointD(-0.23371357519479166176, 60.188217037454911917),
                m2::PointD(-0.27958780433409546884, 60.251555957343796877));
}

UNIT_TEST(PedestrianRouting_UK_Test2)
{
  TestTwoPoints(m2::PointD(-0.23204233496629894651, 60.220733702351964212),
                m2::PointD(-0.25325265780566397211, 60.343129850040341466));
}

UNIT_TEST(PedestrianRouting_UK_Test3)
{
  TestTwoPoints(m2::PointD(-0.13493810466972872009, 60.213290963151536062),
                m2::PointD(-0.075021485248326899575, 60.386990007024301974));
}

UNIT_TEST(PedestrianRouting_UK_Test4)
{
  TestTwoPoints(m2::PointD(0.073624712333011516074, 60.249651023717902376),
                m2::PointD(0.062623007653576381881, 60.305363026945343563));
}

UNIT_TEST(PedestrianRouting_UK_Test5)
{
  TestTwoPoints(m2::PointD(0.073624712333011516074, 60.249651023717902376),
                m2::PointD(0.062623007653576381881, 60.305363026945343563));
}

UNIT_TEST(PedestrianRouting_UK_Test6)
{
  TestTwoPoints(m2::PointD(0.12973003099584515252, 60.286986176872751741),
                m2::PointD(0.16166505598152342005, 60.329896866615413842));
}

UNIT_TEST(PedestrianRouting_UK_Test7)
{
  TestTwoPoints(m2::PointD(0.24339008846246840134, 60.221936300171577727),
                m2::PointD(0.30297476080828561473, 60.472352430858123284));
}

UNIT_TEST(PedestrianRouting_UK_Test8)
{
  TestTwoPoints(m2::PointD(-0.090078418419228770131, 59.938877740935481597),
                m2::PointD(-0.36591832729336593033, 60.383060825937320715));
}

UNIT_TEST(PedestrianRouting_UK_Test9)
{
  TestTwoPoints(m2::PointD(0.013909241828589231221, 60.248524891536746395),
                m2::PointD(-0.011025824403098606619, 60.293190853881299063));
}

UNIT_TEST(PedestrianRouting_UK_Test10)
{
  TestTwoPoints(m2::PointD(-1.2608451895615837568, 60.688400774771103841),
                m2::PointD(-1.3402756985196870865, 60.378654240852370094));
}

UNIT_TEST(PedestrianRouting_UK_Test11)
{
  TestTwoPoints(m2::PointD(-1.2608451895615837568, 60.688400774771103841),
                m2::PointD(-1.3402756985196870865, 60.378654240852370094));
}

UNIT_TEST(PedestrianRouting_UK_Test12)
{
  TestTwoPoints(m2::PointD(-0.41581758334591578663, 60.055074253917027249),
                m2::PointD(-0.0049981648490620023823, 60.559216972985538519));
}

UNIT_TEST(PedestrianRouting_UK_Test13)
{
  TestTwoPoints(m2::PointD(-0.0084726671967171318656, 60.175004410845247094),
                m2::PointD(-0.38290269805087756572, 60.484353263782054455));
}

UNIT_TEST(PedestrianRouting_UK_Test14)
{
  TestTwoPoints(m2::PointD(-0.49920524882713435133, 60.500939921180034275),
                m2::PointD(-0.4253928261485126483, 60.460210120242891207));
}

UNIT_TEST(PedestrianRouting_UK_Test15)
{
  TestTwoPoints(m2::PointD(-0.35293312317744285345, 60.38324360888867659),
                m2::PointD(-0.27232356277650499043, 60.485946731323195991));
}

UNIT_TEST(PedestrianRouting_UK_Test16)
{
  TestTwoPoints(m2::PointD(-0.2452144979288601867, 60.417717669797063706),
                m2::PointD(0.052673435877072988243, 60.48102832828819686));
}

UNIT_TEST(PedestrianRouting_UK_Test17)
{
  TestTwoPoints(m2::PointD(0.60492089858687592141, 60.365652323287299907),
                m2::PointD(0.59411588825676053816, 60.315295907170423106));
}

UNIT_TEST(PedestrianRouting_UK_Test18)
{
  TestTwoPoints(m2::PointD(0.57712031437541466694, 60.311563537302561144),
                m2::PointD(-1.0991154539409491164, 59.24340383025300838));
}

UNIT_TEST(PedestrianRouting_UK_Test19)
{
  TestTwoPoints(m2::PointD(-0.42410951183471667925, 60.225100494175073607),
                m2::PointD(-0.4417841749541066565, 60.377963804987665242));
}

UNIT_TEST(PedestrianRouting_UK_Test20)
{
  TestTwoPoints(m2::PointD(0.087765601941695303712, 60.054331215788280929),
                m2::PointD(0.19336133919110590207, 60.383987006527995334));
}

UNIT_TEST(PedestrianRouting_UK_Test21)
{
  TestTwoPoints(m2::PointD(0.23038165654281794748, 60.438464644310201379),
                m2::PointD(0.18335075596080072091, 60.466925517864886785));
}

UNIT_TEST(PedestrianRouting_UK_Test22)
{
  TestTwoPoints(m2::PointD(-0.33907208409976324903, 60.691735528482595896),
                m2::PointD(-0.17824228031321673327, 60.478512208248780269));
}

UNIT_TEST(PedestrianRouting_UK_Test23)
{
  TestTwoPoints(m2::PointD(-0.0255750943822493082, 60.413717422909641641),
                m2::PointD(0.059727476276875829386, 60.314137951796560344));
}

UNIT_TEST(PedestrianRouting_UK_Test24)
{
  TestTwoPoints(m2::PointD(-0.1251022759281295027, 60.238139790774681614),
                m2::PointD(-0.27656544081449146999, 60.05896703409919013));
}

UNIT_TEST(PedestrianRouting_UK_Test25)
{
  TestTwoPoints(m2::PointD(-0.1251022759281295027, 60.238139790774681614),
                m2::PointD(-0.27656544081449146999, 60.05896703409919013));
}

UNIT_TEST(PedestrianRouting_UK_Test26)
{
  TestTwoPoints(m2::PointD(-3.0453848423988452154, 63.444289157178360483),
                m2::PointD(-2.9888705955481791321, 63.475820316921343078));
}

UNIT_TEST(PedestrianRouting_UK_Test27)
{
  TestTwoPoints(m2::PointD(-2.9465302867103040363, 63.61187786025546842),
                m2::PointD(-2.8321504085699609199, 63.515257402251123153));
}

UNIT_TEST(PedestrianRouting_UK_Test28)
{
  TestTwoPoints(m2::PointD(-2.8527592513387749484, 63.424788250610269813),
                m2::PointD(-2.8824557029010167142, 63.389320899559180589));
}

UNIT_TEST(PedestrianRouting_UK_Test29)
{
  TestTwoPoints(m2::PointD(-2.3526647292757063568, 63.599798938870364395),
                m2::PointD(-2.29857574370878881, 63.546779173754892156));
}

UNIT_TEST(PedestrianRouting_UK_Test30)
{
  TestTwoPoints(m2::PointD(-2.2204371931102926396, 63.410664672405502529),
                m2::PointD(-2.2961918002218593138, 63.653059523966334154));
}

UNIT_TEST(PedestrianRouting_UK_Test31)
{
  TestTwoPoints(m2::PointD(-2.2807869696804585757, 63.6673587499283542),
                m2::PointD(-2.2537861498623481538, 63.627449392852028609));
}
