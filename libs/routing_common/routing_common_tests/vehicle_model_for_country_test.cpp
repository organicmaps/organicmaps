#include "testing/testing.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator_loader.hpp"

#include <memory>
#include <string>

namespace vehicle_model_for_country_test
{
using namespace routing;
using namespace std;

class VehicleModelForCountryTest
{
public:
  VehicleModelForCountryTest() { classificator::Load(); }
};

string GetRegionParent(string const & id)
{
  if (id == "Moscow")
    return "Russian Federation";
  if (id == "Munich")
    return "Bavaria";
  if (id == "Bavaria")
    return "Germany";
  if (id == "San Francisco")
    return "California";
  if (id == "California")
    return "United States of America";
  return "";
}

template <typename VehicleModelType, typename VehicleModelFactoryType>
void TestVehicleModelDefault()
{
  auto defaultVehicleModel = make_shared<VehicleModelType>();

  VehicleModelFactoryType vehicleModelFactory = VehicleModelFactoryType(GetRegionParent);

  // Use static_pointer_cast here because VehicleModelInterface do not have EqualsForTests method
  shared_ptr<VehicleModelType> defaultVehicleModelForCountry =
      static_pointer_cast<VehicleModelType>(vehicleModelFactory.GetVehicleModelForCountry("Nonexistent Country Name"));
  TEST(defaultVehicleModel->EqualsForTests(*defaultVehicleModelForCountry),
       ("Vehicle model for nonexistent counry is not equal to default."));
}

// DirectParent and IndirectParent tests require tested countries to have nondefault restriction
// If selected countries will change their restrictions to default, select other countries for tests
template <typename VehicleModelType, typename VehicleModelFactoryType>
void TestHaveNondefaultRestrictionForSelectedCountry(string country)
{
  auto defaultVehicleModel = make_shared<VehicleModelType>();

  VehicleModelFactoryType vehicleModelFactory = VehicleModelFactoryType(GetRegionParent);

  shared_ptr<VehicleModelType> vehicleModelCountry =
      static_pointer_cast<VehicleModelType>(vehicleModelFactory.GetVehicleModelForCountry(country));

  TEST(!(vehicleModelCountry->EqualsForTests(*defaultVehicleModel)),
       (country,
        "has default model. It may be ok if traffic restrictions was changed. "
        "If so, select other country for this and next test."));
}

template <typename VehicleModelType, typename VehicleModelFactoryType>
void ParentTest(string child, string parent)
{
  VehicleModelFactoryType vehicleModelFactory = VehicleModelFactoryType(GetRegionParent);

  shared_ptr<VehicleModelType> vehicleModelChild =
      static_pointer_cast<VehicleModelType>(vehicleModelFactory.GetVehicleModelForCountry(child));
  shared_ptr<VehicleModelType> vehicleModelParent =
      static_pointer_cast<VehicleModelType>(vehicleModelFactory.GetVehicleModelForCountry(parent));

  TEST(vehicleModelChild->EqualsForTests(*vehicleModelParent), ("Can not expand car model for", child, "to", parent));
}

// Test we have default vehicle models for nonexistent(unknown) country
UNIT_CLASS_TEST(VehicleModelForCountryTest, CarModel_Default)
{
  TestVehicleModelDefault<CarModel, CarModelFactory>();
}

UNIT_CLASS_TEST(VehicleModelForCountryTest, BicycleModel_Default)
{
  TestVehicleModelDefault<BicycleModel, BicycleModelFactory>();
}

UNIT_CLASS_TEST(VehicleModelForCountryTest, PedestrianModel_Default)
{
  TestVehicleModelDefault<PedestrianModel, PedestrianModelFactory>();
}

// 1. Test we have nondefault car model for Russia
// 2. Test we can get car model for Moscow using GetRegionParent callback: car model for Moscow equals
//    car model for Russia and it's not default model.
UNIT_CLASS_TEST(VehicleModelForCountryTest, CarModel_DirectParent)
{
  TestHaveNondefaultRestrictionForSelectedCountry<CarModel, CarModelFactory>("Russian Federation");
  ParentTest<CarModel, CarModelFactory>("Moscow", "Russian Federation");
}

// 1. Test we have nondefault bicycle model for Russia
// 2. Test we can get bicycle model for Moscow using GetRegionParent callback: bicycle model for Moscow
//    equals bicycle model for Russia and it's not default model.
UNIT_CLASS_TEST(VehicleModelForCountryTest, BicycleModel_DirectParent)
{
  // Road types for RF are equal with defaults (speeds are not compared).
  //  TestHaveNondefaultRestrictionForSelectedCountry<BicycleModel, BicycleModelFactory>("Russian Federation");
  ParentTest<BicycleModel, BicycleModelFactory>("Moscow", "Russian Federation");
}

// 1. Test we have nondefault pedestrian model for Russia
// 2. Test we can get pedestrian model for Moscow using GetRegionParent callback: pedestrian model for
//    Moscow equals pedestrian model for Russia and it's not default model.
UNIT_CLASS_TEST(VehicleModelForCountryTest, PedestrianModel_DirectParent)
{
  TestHaveNondefaultRestrictionForSelectedCountry<PedestrianModel, PedestrianModelFactory>("Russian Federation");
  ParentTest<PedestrianModel, PedestrianModelFactory>("Moscow", "Russian Federation");
}

// Test has the same idea as previous one except Germany is not direct parent of Munich
// in GetRegionParent function: Munich -> Bavaria -> Germany
UNIT_CLASS_TEST(VehicleModelForCountryTest, CarModel_InirectParent)
{
  TestHaveNondefaultRestrictionForSelectedCountry<CarModel, CarModelFactory>("Germany");
  ParentTest<CarModel, CarModelFactory>("Munich", "Germany");
}

// Test has the same idea as previous one except United States of America are not direct parent of
// San Francisco in GetRegionParent function: San Francisco -> California -> United States of America
UNIT_CLASS_TEST(VehicleModelForCountryTest, BicycleModel_IndirectParent)
{
  TestHaveNondefaultRestrictionForSelectedCountry<BicycleModel, BicycleModelFactory>("United States of America");
  ParentTest<BicycleModel, BicycleModelFactory>("San Francisco", "United States of America");
}

// Test has the same idea as previous one except United States of America are not direct parent of
// San Francisco in GetRegionParent function: San Francisco -> California -> United States of America
UNIT_CLASS_TEST(VehicleModelForCountryTest, PedestrianModel_IndirectParent)
{
  TestHaveNondefaultRestrictionForSelectedCountry<PedestrianModel, PedestrianModelFactory>("United States of America");
  ParentTest<PedestrianModel, PedestrianModelFactory>("San Francisco", "United States of America");
}
}  // namespace vehicle_model_for_country_test
