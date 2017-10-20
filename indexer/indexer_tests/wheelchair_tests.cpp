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
    WheelchairAvailability a;
    TEST(Wheelchair::GetValue(holder, a), ());
    TEST_EQUAL(a, WheelchairAvailability::No, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "yes"}));
    WheelchairAvailability a;
    TEST(Wheelchair::GetValue(holder, a), ());
    TEST_EQUAL(a, WheelchairAvailability::Yes, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "limited"}));
    WheelchairAvailability a;
    TEST(Wheelchair::GetValue(holder, a), ());
    TEST_EQUAL(a, WheelchairAvailability::Limited, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    WheelchairAvailability a;
    TEST(!Wheelchair::GetValue(holder, a), ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    holder.Add(c.GetTypeByPath({"wheelchair", "yes"}));
    WheelchairAvailability a;
    TEST(Wheelchair::GetValue(holder, a), ());
    TEST_EQUAL(a, WheelchairAvailability::Yes, ());
  }
}
