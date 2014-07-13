#include "../../testing/testing.hpp"

#include "../vehicle_model.hpp"
#include "../../indexer/classificator.hpp"
#include "../../indexer/feature.hpp"
#include "../../base/macros.hpp"

namespace
{

routing::VehicleModel::SpeedForType s_testLimits[] = {
  { {"highway", "trunk"},          150 },
  { {"highway", "primary"},        120 },
  { {"highway", "secondary"},      80 },
  { {"highway", "residential"},    50 },
};

class TestVehicleModel : public routing::VehicleModel
{
public:
  TestVehicleModel()
    : VehicleModel(classif(), vector<VehicleModel::SpeedForType>(s_testLimits, s_testLimits + ARRAY_SIZE(s_testLimits)))
  {
  }
};

uint32_t GetType(char const * s0, char const * s1 = 0, char const * s2 = 0)
{
  char const * const t[] = {s0, s1, s2};
  size_t const size = (s0 != 0) + size_t(s1 != 0) + size_t(s2 != 0);
  return classif().GetTypeByPath(vector<string>(t, t + size));

}

void CheckSpeed(vector<uint32_t> types, double expectedSpeed)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (size_t i = 0; i < types.size(); ++i)
    h(types[i]);
  TEST_EQUAL(vehicleModel.GetSpeed(h), expectedSpeed, ());
}

void CheckOneWay(vector<uint32_t> types, bool expectedValue)
{
  TestVehicleModel vehicleModel;
  feature::TypesHolder h;
  for (size_t i = 0; i < types.size(); ++i)
    h(types[i]);
  TEST_EQUAL(vehicleModel.IsOneWay(h), expectedValue, ());
}


}

UNIT_TEST(VehicleModel_MaxSpeed)
{
  TestVehicleModel vehicleModel;
  TEST_EQUAL(vehicleModel.GetMaxSpeed(), 150, ());
}

UNIT_TEST(VehicleModel_Speed)
{
  CheckSpeed(vector<uint32_t>(1, GetType("highway", "secondary", "bridge")), 80.0);
  CheckSpeed(vector<uint32_t>(1, GetType("highway", "secondary", "tunnel")), 80.0);
  CheckSpeed(vector<uint32_t>(1, GetType("highway", "secondary")), 80.0);
  CheckSpeed(vector<uint32_t>(1, GetType("highway")), 0.0);

  CheckSpeed(vector<uint32_t>(1, GetType("highway", "trunk")), 150.0);
  CheckSpeed(vector<uint32_t>(1, GetType("highway", "primary")), 120.0);
  CheckSpeed(vector<uint32_t>(1, GetType("highway", "residential")), 50.0);
}

UNIT_TEST(VehicleModel_Speed_MultiTypes)
{
  uint32_t const typeTunnel = GetType("highway", "secondary", "tunnel");
  uint32_t const typeSecondary = GetType("highway", "secondary");
  uint32_t const typeHighway = GetType("highway");
  {
    uint32_t types[] = { typeTunnel, typeSecondary };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
  }
  {
    uint32_t types[] = { typeTunnel, typeHighway };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
  }
  {
    uint32_t types[] = { typeHighway, typeTunnel };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
  }
  {
    uint32_t types[] = { typeHighway, typeHighway };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 0.0);
  }
}

UNIT_TEST(VehicleModel_OneWay)
{
  {
    uint32_t types[] = { GetType("highway", "secondary", "bridge"), GetType("oneway") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
    CheckOneWay(vector<uint32_t>(types, types + ARRAY_SIZE(types)), true);
  }
  {
    uint32_t types[] = { GetType("oneway"), GetType("highway", "secondary", "bridge") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
    CheckOneWay(vector<uint32_t>(types, types + ARRAY_SIZE(types)), true);
  }
  {
    uint32_t types[] = { GetType("oneway") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 0.0);
    CheckOneWay(vector<uint32_t>(types, types + ARRAY_SIZE(types)), true);
  }
}

UNIT_TEST(VehicleModel_DifferentSpeeds)
{
  {
    uint32_t types[] = { GetType("highway", "secondary"), GetType("highway", "primary") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
  }
  {
    uint32_t types[] = { GetType("highway", "primary"), GetType("highway", "secondary") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
  }
  {
    uint32_t types[] = { GetType("highway", "primary"), GetType("oneway"), GetType("highway", "secondary") };
    CheckSpeed(vector<uint32_t>(types, types + ARRAY_SIZE(types)), 80.0);
    CheckOneWay(vector<uint32_t>(types, types + ARRAY_SIZE(types)), true);
  }
}
