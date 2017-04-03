#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/facebook_ads.hpp"

namespace
{
UNIT_TEST(Facebook_GetBanner)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::Facebook const facebook;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "Brazil"), "185237551520383_1384652351578891", ());
    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "Cuba"), "185237551520383_1384652351578891", ());
  }
  {
    feature::TypesHolder holder;
    holder.Add(c.GetTypeByPath({"amenity", "restaurant"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "Any country"), "185237551520383_1384650164912443", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "Russia"), "185237551520383_1384651734912286", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "USA"), "185237551520383_1384650804912379", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    auto const bannerId = facebook.GetBannerIdForOtherTypes();
    TEST_EQUAL(facebook.GetBannerId(holder, "Spain"), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    auto const bannerId = facebook.GetBannerIdForOtherTypes();
    TEST_EQUAL(facebook.GetBannerId(holder, "Denmark"), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST_EQUAL(facebook.GetBannerId(holder, "India"), "", ());
  }
}
}  // namespace
