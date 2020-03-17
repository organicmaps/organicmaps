#include "testing/testing.hpp"

#include "partners_api/ads/facebook_ads.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

namespace
{
UNIT_TEST(Facebook_BannerInSearch)
{
  ads::FacebookSearch facebook;
  auto result = facebook.GetBanner();
  TEST_EQUAL(result, "dummy", ());
}

UNIT_TEST(Facebook_GetBanner)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::FacebookPoi facebook;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"Brazil"}, "ru"),
               facebook.GetBannerForOtherTypesForTesting(), ());
    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"Cuba"}, "ru"),
               facebook.GetBannerForOtherTypesForTesting(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Add(c.GetTypeByPath({"amenity", "restaurant"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"Any country"}, "ru"),
               facebook.GetBannerForOtherTypesForTesting(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"Russia"}, "ru"),
               facebook.GetBannerForOtherTypesForTesting(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"USA"}, "ru"),
               facebook.GetBannerForOtherTypesForTesting(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    auto const bannerId = facebook.GetBannerForOtherTypesForTesting();
    TEST_EQUAL(facebook.GetBanner(holder, {"Spain"}, "ru"), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    auto const bannerId = facebook.GetBannerForOtherTypesForTesting();
    TEST_EQUAL(facebook.GetBanner(holder, {"Denmark"}, "ru"), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST_EQUAL(facebook.GetBanner(holder, {"India"}, "ru"), "", ());
  }
}
}  // namespace
