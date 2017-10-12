#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftraits.hpp"

UNIT_TEST(UgcTypes_Full)
{
  classificator::Load();
  Classificator const & c = classif();

  using ftraits::UGC;

  feature::TypesHolder holder;
  {
    holder.Assign(c.GetTypeByPath({"amenity", "bank"}));
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"quality", "service", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"tourism", "information", "office"}));
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"quality", "service", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "hospital"}));
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"expertise", "equipment", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"traffic_calming", "bump"}));
    TEST(!UGC::IsUGCAvailable(holder), ());
  }
}
