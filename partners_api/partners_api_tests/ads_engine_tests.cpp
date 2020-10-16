#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "partners_api/ads/ads_engine.hpp"
#include "partners_api/ads/facebook_ads.hpp"
#include "partners_api/ads/google_ads.hpp"
#include "partners_api/ads/mopub_ads.hpp"
#include "partners_api/ads/rb_ads.hpp"

#include <memory>

namespace
{
class DummyDelegate : public ads::Engine::Delegate
{
public:
  // ads::Engine::Delegate
  bool IsAdsRemoved() const override { return false; }

  // ads::DownloadOnMapContainer::Delegate
  storage::CountryId GetCountryId(m2::PointD const & pos) override { return {}; }
  storage::CountryId GetTopmostParentFor(storage::CountryId const & mwmId) const override { return {}; };
  std::string GetLinkForCountryId(storage::CountryId const & countryId) const override { return {}; };
  std::vector<taxi::Provider::Type> GetTaxiProvidersAtPos(m2::PointD const & pos) const override { return {}; }
};

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
  ads::Engine engine(std::make_unique<DummyDelegate>());
  ads::Mopub mopub;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    auto result = engine.GetPoiBanners(holder, {"Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"7", mopub.GetBannerForOtherTypesForTesting()});

    holder.Add(c.GetTypeByPath({"amenity", "pub"}));
    result = engine.GetPoiBanners(holder, {"Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"7", mopub.GetBannerForOtherTypesForTesting()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"tourism", "information", "map"}));
    auto result = engine.GetPoiBanners(holder, {"Moldova"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"5", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"shop", "ticket"}));
    auto result = engine.GetPoiBanners(holder, {"Russian Federation"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"2", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "bank"}));
    auto result = engine.GetPoiBanners(holder, {"Belarus"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"8", mopub.GetBannerForOtherTypesForTesting()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    auto result = engine.GetPoiBanners(holder, {"Spain", "Ukraine"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"1", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    auto result = engine.GetPoiBanners(holder, {"Ukraine", "Spain"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {"1", "d298f205fb8a47aaafb514d2b5b8cf55"});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "pub"}));
    auto result = engine.GetPoiBanners(holder, {"Spain"}, "en");
    CheckIds(result, {"d298f205fb8a47aaafb514d2b5b8cf55"});
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Mopub, ());
  }
  ads::Rb rb;
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    auto result = engine.GetPoiBanners(holder, {"Armenia"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypesForTesting(), mopub.GetBannerForOtherTypesForTesting()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"amenity", "toilets"}));
    auto result = engine.GetPoiBanners(holder, {"Armenia", "Azerbaijan Region"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypesForTesting(), mopub.GetBannerForOtherTypesForTesting()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    auto result = engine.GetPoiBanners(holder, {"Brazil"}, "en");
    CheckIds(result, {mopub.GetBannerForOtherTypesForTesting()});
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Mopub, ());
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "opentable"}));
    auto result = engine.GetPoiBanners(holder, {"Brazil"}, "ru");
    CheckCountAndTypes(result);
    CheckIds(result, {rb.GetBannerForOtherTypesForTesting(), mopub.GetBannerForOtherTypesForTesting()});
  }
  {
    feature::TypesHolder holder;
    holder.Assign(c.GetTypeByPath({"sponsored", "booking"}));
    auto result = engine.GetPoiBanners(holder, {"Russian Federation"}, "ru");
    TEST(result.empty(), ());
  }
  ads::FacebookSearch facebook;
  {
    auto result = engine.GetSearchBanners();
    TEST_EQUAL(result.size(), 1, ());
    TEST_EQUAL(result[0].m_type, ads::Banner::Type::Facebook, ());
    TEST_EQUAL(result[0].m_value, facebook.GetBanner(), ());
  }
}
}
