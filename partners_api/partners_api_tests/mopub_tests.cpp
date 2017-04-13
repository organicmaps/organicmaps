#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/mopub_ads.hpp"

namespace
{
UNIT_TEST(Mopub_GetBanner)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::Mopub const mopub;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Brazil"), mopub.GetBannerIdForOtherTypes(), ());
    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Cuba"), mopub.GetBannerIdForOtherTypes(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Add(c.GetTypeByPath({"amenity", "restaurant"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Any country"), "d298f205fb8a47aaafb514d2b5b8cf55", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Russia"), "d298f205fb8a47aaafb514d2b5b8cf55", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"highway", "speed_camera"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Egypt"), "fbd54c31a20347a6b5d6654510c542a4", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"building"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Russia"), "fbd54c31a20347a6b5d6654510c542a4", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "USA"), "d298f205fb8a47aaafb514d2b5b8cf55", ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Spain"), mopub.GetBannerIdForOtherTypes(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "Denmark"), mopub.GetBannerIdForOtherTypes(), ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST_EQUAL(mopub.GetBannerId(holder, "India"), "", ());
  }
}
}  // namespace
