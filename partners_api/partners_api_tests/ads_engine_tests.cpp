#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/facebook_ads.hpp"
#include "partners_api/rb_ads.hpp"

namespace
{
void CheckCountAndTypes(std::vector<ads::Banner> const & banners)
{
  TEST_EQUAL(banners.size(), 2, ());
  TEST_EQUAL(banners[0].m_type, ads::Banner::Type::RB, ());
  TEST_EQUAL(banners[1].m_type, ads::Banner::Type::Facebook, ());
}

void CheckIds(std::vector<ads::Banner> const & banners, std::vector<std::string> const & ids)
{
  TEST_EQUAL(banners.size(), ids.size(), ());
  for (size_t i = 0; i < banners.size(); ++i)
  {
    TEST_EQUAL(banners[i].m_bannerId, ids[i], ());
  }
}

UNIT_TEST(AdsEngine_Smoke)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::Engine engine;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {"7", "185237551520383_1384652351578891"});

    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST(engine.HasBanner(holder), ());
    result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {"7", "185237551520383_1384652351578891"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {"5", "185237551520383_1384651734912286"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {"2", "185237551520383_1384650804912379"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "bank"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {"8", "185237551520383_1384652658245527"});
  }
  ads::Rb rb;
  ads::Facebook facebook;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerIdForOtherTypes(), facebook.GetBannerIdForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    TEST(engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerIdForOtherTypes(), facebook.GetBannerIdForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST(!engine.HasBanner(holder), ());
    auto result = engine.GetBanners(holder);
    TEST(result.empty(), ());
  }
}
}
