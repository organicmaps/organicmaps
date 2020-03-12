#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/ads_engine.hpp"
#include "partners_api/facebook_ads.hpp"
#include "partners_api/google_ads.hpp"
#include "partners_api/mopub_ads.hpp"
#include "partners_api/rb_ads.hpp"

namespace
{
void CheckCountAndTypes(std::vector<ads::Banner> const & banners)
{
  TEST_EQUAL(banners.size(), 2, ());
  TEST_EQUAL(banners[0].m_type, ads::Banner::Type::RB, ());
  TEST_EQUAL(banners[1].m_type, ads::Banner::Type::Mopub, ());
}

void CheckIds(std::vector<ads::Banner> const & banners, std::vector<std::string> const & ids)
{
  TEST_EQUAL(banners.size(), ids.size(), ());
  for (size_t i = 0; i < banners.size(); ++i)
  {
    TEST_EQUAL(banners[i].m_value, ids[i], ());
  }
}

UNIT_TEST(AdsEngine_Smoke)
{
  classificator::Load();
  Classificator const & c = classif();
  ads::Engine engine;
  ads::Mopub mopub;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST(engine.HasPoiBanner(holder, {"Ukraine"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"7", mopub.GetBannerForOtherTypes()});

    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    TEST(engine.HasPoiBanner(holder, {"Ukraine"}, "ru"), ());
    result = engine.GetPoiBanners(holder, {"Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"7", mopub.GetBannerForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    TEST(engine.HasPoiBanner(holder, {"Moldova"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Moldova"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"5", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    TEST(engine.HasPoiBanner(holder, {"Russian Federation"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Russian Federation"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"2", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "bank"}));
    TEST(engine.HasPoiBanner(holder, {"Belarus"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Belarus"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"8", mopub.GetBannerForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    TEST(engine.HasPoiBanner(holder, {"Spain", "Ukraine"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Spain", "Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"1", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    TEST(engine.HasPoiBanner(holder, {"Ukraine", "Spain"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Ukraine", "Spain"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"1", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    TEST(engine.HasPoiBanner(holder, {"Spain"}, "en"), ());
    auto result = engine.GetPoiBanners(holder, {"Spain"}, "en");
    CheckIds(result, {"d298f205fb8a47aaafb514d2b5b8cf55"});
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Mopub, ());
  }
  ads::Rb rb;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    TEST(engine.HasPoiBanner(holder, {"Armenia"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Armenia"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypes(), mopub.GetBannerForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    TEST(engine.HasPoiBanner(holder, {"Armenia", "Azerbaijan Region"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Armenia", "Azerbaijan Region"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypes(), mopub.GetBannerForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    TEST(engine.HasPoiBanner(holder, {"Brazil"}, "en"), ());
    auto result = engine.GetPoiBanners(holder, {"Brazil"}, "en");
    CheckIds(result, {mopub.GetBannerForOtherTypes()});
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Mopub, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    TEST(engine.HasPoiBanner(holder, {"Brazil"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Brazil"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypes(), mopub.GetBannerForOtherTypes()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    TEST(!engine.HasPoiBanner(holder, {"Russian Federation"}, "ru"), ());
    auto result = engine.GetPoiBanners(holder, {"Russian Federation"}, "ru");
    TEST(result.empty(), ());
  }
  ads::FacebookSearch facebook;
  {
    TEST(engine.HasSearchBanner(), ());
    auto result = engine.GetSearchBanners();
    TEST_EQUAL(result.size(), 1, ());
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Facebook, ());
    TEST_EQUAL(result[0].m_value, facebook.GetBanner(), ());
  }
}
}
