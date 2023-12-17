#include "testing/testing.hpp"

#include "routing/routing_benchmarks/helpers.hpp"

#include "routing/pedestrian_directions.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/pedestrian_model.hpp"

#include <memory>
#include <set>
#include <string>
#include <utility>

namespace pedestrian_routing_tests
{
using namespace std;

// Test preconditions: files from the kPedestrianMapFiles set with '.mwm'
// extension must be placed in omim/data folder.
set<string> const kPedestrianMapFiles =
{
  "UK_England_East Midlands",
  "UK_England_East of England_Essex",
  "UK_England_East of England_Norfolk",
  "UK_England_Greater London",
  "UK_England_North East England",
  "UK_England_North West England_Lancaster",
  "UK_England_North West England_Manchester",
  "UK_England_South East_Brighton",
  "UK_England_South East_Oxford",
  "UK_England_South West England_Bristol",
  "UK_England_South West England_Cornwall",
  "UK_England_West Midlands",
  "UK_England_Yorkshire and the Humber"
};

class PedestrianTest : public RoutingTest
{
public:
  PedestrianTest()
    : RoutingTest(routing::IRoadGraph::Mode::IgnoreOnewayTag, routing::VehicleType::Pedestrian,
                  kPedestrianMapFiles)
  {
  }

protected:
  unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() override
  {
    unique_ptr<routing::VehicleModelFactoryInterface> factory(
        new SimplifiedModelFactory<routing::PedestrianModel>());
    return factory;
  }
};

// Tests on features -------------------------------------------------------------------------------
UNIT_CLASS_TEST(PedestrianTest, UK_Long1)
{
  TestTwoPointsOnFeature(m2::PointD(-1.88798, 61.90292), m2::PointD(-2.06025, 61.82824));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Long2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.20434, 60.27445), m2::PointD(0.06962, 60.33909));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Long3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.07706, 60.42876), m2::PointD(-0.11058, 60.20991));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Long4)
{
  TestTwoPointsOnFeature(m2::PointD(-0.48574, 60.05082), m2::PointD(-0.45973, 60.56715));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Long5)
{
  TestTwoPointsOnFeature(m2::PointD(0.11646, 60.57330), m2::PointD(0.05767, 59.93019));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Long6)
{
  TestTwoPointsOnFeature(m2::PointD(0.02771, 60.49348), m2::PointD(0.06533, 59.93155));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Medium1)
{
  TestTwoPointsOnFeature(m2::PointD(-0.10461, 60.29721), m2::PointD(-0.07532, 60.35180));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Medium2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.17925, 60.06331), m2::PointD(-0.09959, 60.06880));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Medium3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.26440, 60.16831), m2::PointD(-0.20113, 60.20884));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Medium4)
{
  TestTwoPointsOnFeature(m2::PointD(-0.25296, 60.46539), m2::PointD(-0.10975, 60.43955));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Medium5)
{
  TestTwoPointsOnFeature(m2::PointD(-0.03115, 60.31819), m2::PointD(0.07400, 60.33662));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Short1)
{
  TestTwoPointsOnFeature(m2::PointD(-0.10461, 60.29721), m2::PointD(-0.11905, 60.29747));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Short2)
{
  TestTwoPointsOnFeature(m2::PointD(-0.11092, 60.27172), m2::PointD(-0.08159, 60.27623));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Short3)
{
  TestTwoPointsOnFeature(m2::PointD(-0.09449, 60.25051), m2::PointD(-0.06520, 60.26647));
}

// Tests on points ---------------------------------------------------------------------------------
UNIT_CLASS_TEST(PedestrianTest, UK_Test1)
{
  TestRouters(m2::PointD(-0.23371, 60.18821), m2::PointD(-0.27958, 60.25155));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test2)
{
  TestRouters(m2::PointD(-0.23204, 60.22073), m2::PointD(-0.25325, 60.34312));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test3)
{
  TestRouters(m2::PointD(-0.13493, 60.21329), m2::PointD(-0.07502, 60.38699));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test4)
{
  TestRouters(m2::PointD(0.07362, 60.24965), m2::PointD(0.06262, 60.30536));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test6)
{
  TestRouters(m2::PointD(0.12973, 60.28698), m2::PointD(0.16166, 60.32989));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test7)
{
  TestRouters(m2::PointD(0.24339, 60.22193), m2::PointD(0.30297, 60.47235));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test9)
{
  TestRouters(m2::PointD(0.01390, 60.24852), m2::PointD(-0.01102, 60.29319));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test10)
{
  TestRouters(m2::PointD(-1.26084, 60.68840), m2::PointD(-1.34027, 60.37865));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test11)
{
  TestRouters(m2::PointD(-1.26084, 60.68840), m2::PointD(-1.34027, 60.37865));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test14)
{
  TestRouters(m2::PointD(-0.49921, 60.50093), m2::PointD(-0.42539, 60.46021));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test15)
{
  TestRouters(m2::PointD(-0.35293, 60.38324), m2::PointD(-0.27232, 60.48594));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test16)
{
  TestRouters(m2::PointD(-0.24521, 60.41771), m2::PointD(0.052673, 60.48102));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test17)
{
  TestRouters(m2::PointD(0.60492, 60.36565), m2::PointD(0.59411, 60.31529));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test19)
{
  TestRouters(m2::PointD(-0.42411, 60.22511), m2::PointD(-0.44178, 60.37796));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test20)
{
  TestRouters(m2::PointD(0.08776, 60.05433), m2::PointD(0.19336, 60.38398));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test21)
{
  TestRouters(m2::PointD(0.23038, 60.43846), m2::PointD(0.18335, 60.46692));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test22)
{
  TestRouters(m2::PointD(-0.33907, 60.691735), m2::PointD(-0.17824, 60.478512));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test23)
{
  TestRouters(m2::PointD(-0.02557, 60.41371), m2::PointD(0.05972, 60.31413));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test24)
{
  TestRouters(m2::PointD(-0.12511, 60.23813), m2::PointD(-0.27656, 60.05896));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test25)
{
  TestRouters(m2::PointD(-0.12511, 60.23813), m2::PointD(-0.27656, 60.05896));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test26)
{
  TestRouters(m2::PointD(-3.04538, 63.44428), m2::PointD(-2.98887, 63.47582));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test27)
{
  TestRouters(m2::PointD(-2.94653, 63.61187), m2::PointD(-2.83215, 63.51525));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test28)
{
  TestRouters(m2::PointD(-2.85275, 63.42478), m2::PointD(-2.88245, 63.38932));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test29)
{
  TestRouters(m2::PointD(-2.35266, 63.59979), m2::PointD(-2.29857, 63.54677));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test30)
{
  TestRouters(m2::PointD(-2.22043, 63.41066), m2::PointD(-2.29619, 63.65305));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test31)
{
  TestRouters(m2::PointD(-2.28078, 63.66735), m2::PointD(-2.25378, 63.62744));
}

// This is very slow pedestrian tests (more than 20 minutes).
#if defined(SLOW_TESTS)
UNIT_CLASS_TEST(PedestrianTest, UK_Test5)
{
  TestRouters(m2::PointD(0.07362, 60.24965), m2::PointD(0.06262, 60.30536));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test8)
{
  TestRouters(m2::PointD(-0.09007, 59.93887), m2::PointD(-0.36591, 60.38306));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test12)
{
  TestRouters(m2::PointD(-0.41581, 60.05507), m2::PointD(-0.00499, 60.55921));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test13)
{
  TestRouters(m2::PointD(-0.00847, 60.17501), m2::PointD(-0.38291, 60.48435));
}

UNIT_CLASS_TEST(PedestrianTest, UK_Test18)
{
  TestTwoPoints(m2::PointD(0.57712, 60.31156), m2::PointD(-1.09911, 59.24341));
}
#endif
}  // namespace pedestrian_routing_tests
