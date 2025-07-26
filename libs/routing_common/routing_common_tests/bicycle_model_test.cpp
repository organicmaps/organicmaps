#include "testing/testing.hpp"

#include "routing_common/bicycle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

namespace bicycle_model_test
{
using namespace routing;

class BicycleModelTest
{
public:
  BicycleModelTest() { classificator::Load(); }

  std::shared_ptr<VehicleModel> GetModel(std::string const & country)
  {
    return std::dynamic_pointer_cast<VehicleModel>(BicycleModelFactory().GetVehicleModelForCountry(country));
  }

  SpeedParams DefaultSpeedParams() { return {true /* forward */, true /* isCity */, Maxspeed()}; }
};

UNIT_CLASS_TEST(BicycleModelTest, Turkey)
{
  auto const model = GetModel("Turkey");
  TEST(model, ());
  auto const & cl = classif();

  feature::TypesHolder holder;
  holder.Add(cl.GetTypeByPath({"highway", "footway", "tunnel"}));

  TEST(model->HasRoadType(holder), ());
  TEST_EQUAL(model->GetSpeed(holder, DefaultSpeedParams()), BicycleModel::DismountSpeed(), ());
}

}  // namespace bicycle_model_test
