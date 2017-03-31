#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/rb_ads.hpp"

namespace
{
UNIT_TEST(Rb_GetBanner)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::Rb const rb;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST_EQUAL(rb.GetBannerId(holder), "7", ());
    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST_EQUAL(rb.GetBannerId(holder), "7", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "restaurant"}));
    TEST_EQUAL(rb.GetBannerId(holder), "1", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST_EQUAL(rb.GetBannerId(holder), "5", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST_EQUAL(rb.GetBannerId(holder), "2", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "bank"}));
    TEST_EQUAL(rb.GetBannerId(holder), "8", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "atm"}));
    TEST_EQUAL(rb.GetBannerId(holder), "8", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    auto const bannerId = rb.GetBannerIdForOtherTypes();
    TEST_EQUAL(rb.GetBannerId(holder), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    auto const bannerId = rb.GetBannerIdForOtherTypes();
    TEST_EQUAL(rb.GetBannerId(holder), bannerId, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST_EQUAL(rb.GetBannerId(holder), "", ());
  }
}
}  // namespace
