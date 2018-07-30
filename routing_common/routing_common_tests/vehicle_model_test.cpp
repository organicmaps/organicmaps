#include "testing/testing.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"

#include "base/macros.hpp"

using namespace std;

namespace
{
routing::VehicleModel::LimitsInitList const s_testLimits = {
    {{"highway", "trunk"}, {150 /* weightSpeed */, 150 /* etaSpeed */}, true},
    {{"highway", "primary"}, {120, 120}, true},
    {{"highway", "secondary"}, {80, 70}, true},
    {{"highway", "residential"}, {50, 60}, true},
    {{"highway", "service"}, {50, 40}, false}
};

routing::VehicleModel::SurfaceInitList const g_carSurface = {
    {{"psurface", "paved_good"}, {0.8 /* weightFactor */, 0.9 /* etaFactor */}},
    {{"psurface", "paved_bad"}, {0.4, 0.5}},
    {{"psurface", "unpaved_good"}, {0.6, 0.8}},
    {{"psurface", "unpaved_bad"}, {0.2, 0.2}}
};

class VehicleModelTest
{
public:
  VehicleModelTest() { classificator::Load(); }
};

class TestVehicleModel : public routing::VehicleModel
{
  friend void CheckOneWay(initializer_list<uint32_t> const & types, bool expectedValue);
  friend void CheckPassThroughAllowed(initializer_list<uint32_t> const & types, bool expectedValue);
  friend void CheckSpeed(initializer_list<uint32_t> const & types,
                         routing::VehicleModelInterface::SpeedKMpH && expectedSpeed);

public:
  TestVehicleModel() : VehicleModel(classif(), s_testLimits, g_carSurface) {}

  // We are not going to use offroad routing in these tests.
  double GetOffroadSpeed() const override { return 0.0; }
};

uint32_t GetType(char const * s0, char const * s1 = 0, char const * s2 = 0)
{
  char const * const t[] = {s0, s1, s2};
  size_t const size = (s0 != 0) + size_t(s1 != 0) + size_t(s2 != 0);
  return classif().GetTypeByPath(vector<string>(t, t + size));
}

uint32_t GetOnewayType()
{
  return GetType("hwtag", "oneway");
}

void CheckSpeed(initializer_list<uint32_t> const & types, routing::VehicleModelInterface::SpeedKMpH && expectedSpeed)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(vehicleModel.GetMinTypeSpeed(h), expectedSpeed, ());
}

void CheckOneWay(initializer_list<uint32_t> const & types, bool expectedValue)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(vehicleModel.HasOneWayType(h), expectedValue, ());
}

void CheckPassThroughAllowed(initializer_list<uint32_t> const & types, bool expectedValue)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(vehicleModel.HasPassThroughType(h), expectedValue, ());
}
}  // namespace

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_MaxSpeed)
{
  TestVehicleModel vehicleModel;
  TEST_EQUAL(vehicleModel.GetMaxSpeed().m_weight, 150, ());
  TEST_EQUAL(vehicleModel.GetMaxSpeed().m_eta, 150, ());
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed)
{
  CheckSpeed({GetType("highway", "secondary", "bridge")}, {80.0 /* weightSpeed KMpH */, 70.0 /* etaSpeed KMpH */});
  CheckSpeed({GetType("highway", "secondary", "tunnel")}, {80.0, 70.0});
  CheckSpeed({GetType("highway", "secondary")}, {80.0, 70.0});

  CheckSpeed({GetType("highway", "trunk")}, {150.0, 150.0});
  CheckSpeed({GetType("highway", "primary")}, {120.0, 120.0});
  CheckSpeed({GetType("highway", "residential")}, {50.0, 60.});
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed_MultiTypes)
{
  uint32_t const typeTunnel = GetType("highway", "secondary", "tunnel");
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typeHighway = GetType("highway");

  CheckSpeed({typeTunnel, typeSecondary}, {80.0 /* weightSpeed KMpH */, 70.0 /* etaSpeed, KMpH */});
  CheckSpeed({typeTunnel, typeHighway}, {80.0, 70.0});
  CheckSpeed({typeHighway, typeTunnel}, {80.0, 70.0});
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_OneWay)
{
  uint32_t const typeBridge = GetType("highway", "secondary", "bridge");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeBridge, typeOneway}, {80.0 /* weightSpeed KMpH */, 70.0 /* etaSpeed KMpH */});
  CheckOneWay({typeBridge, typeOneway}, true);
  CheckSpeed({typeOneway, typeBridge}, {80.0, 70.0});
  CheckOneWay({typeOneway, typeBridge}, true);

  CheckOneWay({typeOneway}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_DifferentSpeeds)
{
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typePrimary = GetType("highway", "primary");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeSecondary, typePrimary}, {80.0 /* weightSpeed KMpH */, 70.0 /* etaSpeed KMpH */});
  CheckSpeed({typePrimary, typeSecondary}, {80.0, 70.0});

  CheckSpeed({typePrimary, typeOneway, typeSecondary}, {80.0, 70.0});
  CheckOneWay({typePrimary, typeOneway, typeSecondary}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_PassThroughAllowed)
{
  CheckPassThroughAllowed({GetType("highway", "secondary")}, true);
  CheckPassThroughAllowed({GetType("highway", "primary")}, true);
  CheckPassThroughAllowed({GetType("highway", "service")}, false);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_SpeedFactor)
{
  uint32_t const secondary = GetType("highway", "secondary");
  uint32_t const residential = GetType("highway", "residential");
  uint32_t const pavedGood = GetType("psurface", "paved_good");
  uint32_t const pavedBad = GetType("psurface", "paved_bad");
  uint32_t const unpavedGood = GetType("psurface", "unpaved_good");
  uint32_t const unpavedBad = GetType("psurface", "unpaved_bad");

  CheckSpeed({secondary, pavedGood}, {64.0 /* weightSpeed KMpH */, 63.0 /* weightSpeed KMpH */});
  CheckSpeed({secondary, pavedBad}, {32.0, 35.0});
  CheckSpeed({secondary, unpavedGood}, {48.0, 56.0});
  CheckSpeed({secondary, unpavedBad}, {16.0, 14.0});

  CheckSpeed({residential, pavedGood}, {40.0, 54.0});
  CheckSpeed({residential, pavedBad}, {20.0, 30.0});
  CheckSpeed({residential, unpavedGood}, {30.0, 48.0});
  CheckSpeed({residential, unpavedBad}, {10.0, 12.0});
}
