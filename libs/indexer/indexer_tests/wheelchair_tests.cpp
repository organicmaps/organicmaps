#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftraits.hpp"

UNIT_TEST(Wheelchair_GetType)
{
  classificator::Load();
  Classificator const & c = classif();

  using ftraits::Wheelchair;
  using ftraits::WheelchairAvailability;

  feature::TypesHolder holder;
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "no"}));
    TEST_EQUAL(*Wheelchair::GetValue(holder), WheelchairAvailability::No, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "yes"}));
    TEST_EQUAL(*Wheelchair::GetValue(holder), WheelchairAvailability::Yes, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "limited"}));
    TEST_EQUAL(*Wheelchair::GetValue(holder), WheelchairAvailability::Limited, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST(!Wheelchair::GetValue(holder), ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    holder.Add(c.GetTypeByPath({"wheelchair", "yes"}));
    TEST_EQUAL(*Wheelchair::GetValue(holder), WheelchairAvailability::Yes, ());
  }
}
