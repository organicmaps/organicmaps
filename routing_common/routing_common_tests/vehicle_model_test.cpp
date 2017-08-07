#include "testing/testing.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"

#include "base/macros.hpp"

namespace
{
routing::VehicleModel::InitListT const s_testLimits = {
    {{"highway", "trunk"}, 150, true},
    {{"highway", "primary"}, 120, true},
    {{"highway", "secondary"}, 80, true},
    {{"highway", "residential"}, 50, true},
    {{"highway", "service"}, 50, false},
};

class  VehicleModelTest
{
public:
  VehicleModelTest() { classificator::Load(); }
};

class TestVehicleModel : public routing::VehicleModel
{
  friend void CheckOneWay(initializer_list<uint32_t> const & types, bool expectedValue);
  friend void CheckTransitAllowed(initializer_list<uint32_t> const & types, bool expectedValue);
  friend void CheckSpeed(initializer_list<uint32_t> const & types, double expectedSpeed);

public:
  TestVehicleModel() : VehicleModel(classif(), s_testLimits) {}
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

void CheckSpeed(initializer_list<uint32_t> const & types, double expectedSpeed)
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

void CheckTransitAllowed(initializer_list<uint32_t> const & types, bool expectedValue)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (uint32_t t : types)
    h.Add(t);

  TEST_EQUAL(vehicleModel.HasTransitType(h), expectedValue, ());
}
}  // namespace

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_MaxSpeed)
{
  TestVehicleModel vehicleModel;
  TEST_EQUAL(vehicleModel.GetMaxSpeed(), 150, ());
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed)
{
  CheckSpeed({GetType("highway", "secondary", "bridge")}, 80.0);
  CheckSpeed({GetType("highway", "secondary", "tunnel")}, 80.0);
  CheckSpeed({GetType("highway", "secondary")}, 80.0);

  CheckSpeed({GetType("highway", "trunk")}, 150.0);
  CheckSpeed({GetType("highway", "primary")}, 120.0);
  CheckSpeed({GetType("highway", "residential")}, 50.0);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_Speed_MultiTypes)
{
  uint32_t const typeTunnel = GetType("highway", "secondary", "tunnel");
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typeHighway = GetType("highway");

  CheckSpeed({typeTunnel, typeSecondary}, 80.0);
  CheckSpeed({typeTunnel, typeHighway}, 80.0);
  CheckSpeed({typeHighway, typeTunnel}, 80.0);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_OneWay)
{
  uint32_t const typeBridge = GetType("highway", "secondary", "bridge");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeBridge, typeOneway}, 80.0);
  CheckOneWay({typeBridge, typeOneway}, true);
  CheckSpeed({typeOneway, typeBridge}, 80.0);
  CheckOneWay({typeOneway, typeBridge}, true);

  CheckOneWay({typeOneway}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_DifferentSpeeds)
{
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typePrimary = GetType("highway", "primary");
  uint32_t const typeOneway = GetOnewayType();

  CheckSpeed({typeSecondary, typePrimary}, 80.0);
  CheckSpeed({typePrimary, typeSecondary}, 80.0);

  CheckSpeed({typePrimary, typeOneway, typeSecondary}, 80.0);
  CheckOneWay({typePrimary, typeOneway, typeSecondary}, true);
}

UNIT_CLASS_TEST(VehicleModelTest, VehicleModel_TransitAllowed)
{
  CheckTransitAllowed({GetType("highway", "secondary")}, true);
  CheckTransitAllowed({GetType("highway", "primary")}, true);
  CheckTransitAllowed({GetType("highway", "service")}, false);
}
