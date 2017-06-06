#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/wheelchair.hpp"

UNIT_TEST(Wheelchair_GetType)
{
  classificator::Load();
  Classificator const & c = classif();

  feature::TypesHolder holder;
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "no"}));
    TEST_EQUAL(wheelchair::Matcher::GetType(holder), wheelchair::Type::No, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "yes"}));
    TEST_EQUAL(wheelchair::Matcher::GetType(holder), wheelchair::Type::Yes, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"wheelchair", "limited"}));
    TEST_EQUAL(wheelchair::Matcher::GetType(holder), wheelchair::Type::Limited, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST_EQUAL(wheelchair::Matcher::GetType(holder), wheelchair::Type::No, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    holder.Add(c.GetTypeByPath({"wheelchair", "yes"}));
    TEST_EQUAL(wheelchair::Matcher::GetType(holder), wheelchair::Type::Yes, ());
  }
}
