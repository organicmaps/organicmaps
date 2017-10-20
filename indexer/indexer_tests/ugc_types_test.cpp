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
    auto const type = c.GetTypeByPath({"amenity", "bank"});
    holder.Assign(type);
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"quality", "service", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    uint32_t matchingType;
    TEST(UGC::GetType(holder, matchingType), ());
    TEST_EQUAL(matchingType, type, ());
    TEST_EQUAL(c.GetReadableObjectName(matchingType), "amenity-bank", ());
  }
  {
    auto const type = c.GetTypeByPath({"tourism", "information", "office"});
    holder.Assign(type);
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"quality", "service", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    uint32_t matchingType;
    TEST(UGC::GetType(holder, matchingType), ());
    TEST_EQUAL(matchingType, type, ());
    TEST_EQUAL(c.GetReadableObjectName(matchingType), "tourism-information-office", ());
  }
  {
    auto const type = c.GetTypeByPath({"amenity", "hospital"});
    holder.Assign(type);
    TEST(UGC::IsUGCAvailable(holder), ());
    TEST(UGC::IsRatingAvailable(holder), ());
    TEST(UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {"expertise", "equipment", "value_for_money"};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    uint32_t matchingType;
    TEST(UGC::GetType(holder, matchingType), ());
    TEST_EQUAL(matchingType, type, ());
    TEST_EQUAL(c.GetReadableObjectName(matchingType), "amenity-hospital", ());
  }
  {
    holder.Assign(c.GetTypeByPath({"traffic_calming", "bump"}));
    TEST(!UGC::IsUGCAvailable(holder), ());
    uint32_t matchingType;
    TEST(!UGC::GetType(holder, matchingType), ());
  }
  {
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST(!UGC::IsUGCAvailable(holder), ());
    TEST(!UGC::IsRatingAvailable(holder), ());
    TEST(!UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    ftraits::UGCRatingCategories expected = {};
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    uint32_t matchingType;
    TEST(!UGC::GetType(holder, matchingType), ());

    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    holder.Add(c.GetTypeByPath({"amenity", "hospital"}));
    TEST(!UGC::IsUGCAvailable(holder), ());
    TEST(!UGC::IsRatingAvailable(holder), ());
    TEST(!UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    TEST(!UGC::GetType(holder, matchingType), ());

    holder.Assign(c.GetTypeByPath({"amenity", "hospital"}));
    holder.Add(c.GetTypeByPath({"sponsored", "booking"}));
    TEST(!UGC::IsUGCAvailable(holder), ());
    TEST(!UGC::IsRatingAvailable(holder), ());
    TEST(!UGC::IsReviewsAvailable(holder), ());
    TEST(!UGC::IsDetailsAvailable(holder), ());
    TEST_EQUAL(UGC::GetCategories(holder), expected, ());
    TEST(!UGC::GetType(holder, matchingType), ());
  }
}
