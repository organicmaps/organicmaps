#include "testing/testing.hpp"

#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"

#include "platform/measurement_utils.hpp"

#include "base/macros.hpp"

#include <cstdint>

using namespace routing;
using namespace std;

using MaxspeedFactor = VehicleModel::MaxspeedFactor;

namespace
{
using SpeedKMpH = routing::VehicleModel::SpeedKMpH;
using InOutCitySpeedKMpH = routing::VehicleModel::InOutCitySpeedKMpH;

InOutCitySpeedKMpH const speedSecondaryExpected = {
    SpeedKMpH(80.0 /* weight */, 70.0 /* eta */) /* in city */,
    SpeedKMpH(80.0 /* weight */, 70.0 /* eta */) /* out of city */, MaxspeedFactor(1.0)};

routing::VehicleModel::LimitsInitList const s_testLimits = {
    {{"highway", "trunk"}, {SpeedKMpH(100.0 /* weight */, 100.0 /* eta */) /* in city */,
                            SpeedKMpH(150.0 /* weight */, 150.0 /* eta */) /* out of city */,
                            MaxspeedFactor(1.0)}, true},
    {{"highway", "primary"}, {SpeedKMpH(90.0, 90.0), SpeedKMpH(120.0, 120.0), MaxspeedFactor(1.0)}, true},
    {{"highway", "secondary"}, speedSecondaryExpected, true},
    {{"highway", "residential"}, {SpeedKMpH(45.0, 55.0), SpeedKMpH(50.0, 60.0), MaxspeedFactor(0.5)}, true},
    {{"highway", "service"}, {SpeedKMpH(47.0, 36.0), SpeedKMpH(50.0, 40.0), MaxspeedFactor(1.0)}, false}};

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
  friend void CheckSpeedWithParams(initializer_list<uint32_t> const & types,
                                   SpeedParams const & params, SpeedKMpH const & expectedSpeed);

      public : TestVehicleModel()
    : VehicleModel(classif(), s_testLimits, g_carSurface)
  {
  }

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

void CheckSpeedWithParams(initializer_list<uint32_t> const & types, SpeedParams const & params,
                          SpeedKMpH const & expectedSpeed)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(vehicleModel.GetTypeSpeed(h, params), expectedSpeed, ());
}

void CheckSpeed(initializer_list<uint32_t> const & types, InOutCitySpeedKMpH const & expectedSpeed)
{
  SpeedParams const inCity(true /* forward */, true /* in city */, Maxspeed());
  CheckSpeedWithParams(types, inCity, expectedSpeed.m_inCity);
  SpeedParams const outCity(true /* forward */, false /* in city */, Maxspeed());
  CheckSpeedWithParams(types, outCity, expectedSpeed.m_outCity);
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
  TEST_EQUAL(vehicleModel.GetMaxWeightSpeed(), 150.0, ());
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed)
{
  {
    CheckSpeed({GetType("highway", "secondary", "bridge")}, speedSecondaryExpected);
    CheckSpeed({GetType("highway", "secondary", "tunnel")}, speedSecondaryExpected);
    CheckSpeed({GetType("highway", "secondary")}, speedSecondaryExpected);
  }

  CheckSpeed({GetType("highway", "trunk")},
             {SpeedKMpH(100.0 /* weight */, 100.0 /* eta */) /* in city */,
              SpeedKMpH(150.0 /* weight */, 150.0 /* eta */) /* out of city */,
              MaxspeedFactor(1.0)});
  CheckSpeed({GetType("highway", "primary")},
             {SpeedKMpH(90.0, 90.0), SpeedKMpH(120.0, 120.0), MaxspeedFactor(1.0)});
  CheckSpeed({GetType("highway", "residential")},
             {SpeedKMpH(45.0, 55.0), SpeedKMpH(50.0, 60.0), MaxspeedFactor(1.0)});
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed_MultiTypes)
{
  uint32_t const typeTunnel = GetType("highway", "secondary", "tunnel");
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typeHighway = GetType("highway");

  CheckSpeed({typeTunnel, typeSecondary}, speedSecondaryExpected);
  CheckSpeed({typeTunnel, typeHighway}, speedSecondaryExpected);
  CheckSpeed({typeHighway, typeTunnel}, speedSecondaryExpected);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_OneWay)
{
  uint32_t const typeBridge = GetType("highway", "secondary", "bridge");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeBridge, typeOneway}, speedSecondaryExpected);
  CheckOneWay({typeBridge, typeOneway}, true);
  CheckSpeed({typeOneway, typeBridge}, speedSecondaryExpected);
  CheckOneWay({typeOneway, typeBridge}, true);

  CheckOneWay({typeOneway}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_DifferentSpeeds)
{
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typePrimary = GetType("highway", "primary");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeSecondary, typePrimary}, speedSecondaryExpected);

  CheckSpeed({typeSecondary, typePrimary, typeOneway}, speedSecondaryExpected);
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

  CheckSpeed({secondary, pavedGood}, {SpeedKMpH(64.0 /* weight */, 63.0 /* eta */) /* in city */,
                                      SpeedKMpH(64.0 /* weight */, 63.0 /* eta */) /* out of city */,
                                      MaxspeedFactor(1.0)});
  CheckSpeed({secondary, pavedBad}, {SpeedKMpH(32.0, 35.0), SpeedKMpH(32.0, 35.0), MaxspeedFactor(1.0)});
  CheckSpeed({secondary, unpavedGood}, {SpeedKMpH(48.0, 56.0), SpeedKMpH(48.0, 56.0), MaxspeedFactor(1.0)});
  CheckSpeed({secondary, unpavedBad}, {SpeedKMpH(16.0, 14.0), SpeedKMpH(16.0, 14.0), MaxspeedFactor(1.0)});

  CheckSpeed({residential, pavedGood}, {SpeedKMpH(36.0, 49.5), SpeedKMpH(40.0, 54.0), MaxspeedFactor(1.0)});
  CheckSpeed({residential, pavedBad}, {SpeedKMpH(18.0, 27.5), SpeedKMpH(20.0, 30.0), MaxspeedFactor(1.0)});
  CheckSpeed({residential, unpavedGood}, {SpeedKMpH(27.0, 44.0), SpeedKMpH(30.0, 48.0), MaxspeedFactor(1.0)});
  CheckSpeed({residential, unpavedBad}, {SpeedKMpH(9.0, 11.0), SpeedKMpH(10.0, 12.0), MaxspeedFactor(1.0)});
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_MaxspeedFactor)
{
  uint32_t const secondary = GetType("highway", "secondary");
  uint32_t const residential = GetType("highway", "residential");
  uint32_t const primary = GetType("highway", "primary");
  uint32_t const pavedGood = GetType("psurface", "paved_good");
  uint32_t const unpavedBad = GetType("psurface", "unpaved_bad");

  Maxspeed const maxspeed90 =
      Maxspeed(measurement_utils::Units::Metric, 90 /* forward speed */, kInvalidSpeed);
  CheckSpeedWithParams({secondary, unpavedBad},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed90),
                       SpeedKMpH(90.0));
  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed90),
                       SpeedKMpH(90.0));

  Maxspeed const maxspeed9070 =
      Maxspeed(measurement_utils::Units::Metric, 90 /* forward speed */, 70);
  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed9070),
                       SpeedKMpH(90.0));
  CheckSpeedWithParams({primary, pavedGood},
                       SpeedParams(false /* forward */, false /* in city */, maxspeed9070),
                       SpeedKMpH(70.0));

  Maxspeed const maxspeed60 =
      Maxspeed(measurement_utils::Units::Metric, 60 /* forward speed */, kInvalidSpeed);
  CheckSpeedWithParams({residential, pavedGood},
                       SpeedParams(true /* forward */, false /* in city */, maxspeed60),
                       SpeedKMpH(30.0));
}
