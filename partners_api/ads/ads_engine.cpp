#include "partners_api/ads/ads_engine.hpp"

#include "partners_api/ads/arsenal_ads.hpp"
#include "partners_api/ads/bookmark_catalog_ads.hpp"
#include "partners_api/ads/citymobil_ads.hpp"
#include "partners_api/ads/facebook_ads.hpp"
#include "partners_api/ads/mopub_ads.hpp"
#include "partners_api/ads/rb_ads.hpp"

#include "indexer/feature_data.hpp"

#include <memory>
#include <utility>

namespace
{
template <typename T, typename... Args>
std::vector<ads::Banner> GetBanners(T const & bannerContainer, bool isAdsRemoved,
                                    Args const &... params)
{
  if (isAdsRemoved)
    return {};

  std::vector<ads::Banner> result;

  for (auto const & item : bannerContainer)
  {
    if (!item.m_enabled)
      continue;

    auto const bannerId = item.m_container->GetBanner(params...);
    if (!bannerId.empty())
    {
      result.emplace_back(item.m_type, bannerId);
    }
  }

  return result;
}
}  // namespace
namespace ads
{
Engine::Engine(std::unique_ptr<Delegate> delegate)
  : m_delegate(std::move(delegate))
{
  ASSERT(m_delegate != nullptr, ());
  // The banner systems are placed by priority. First has a top priority.
  m_poiBanners.emplace_back(Banner::Type::RB, std::make_unique<Rb>());
  m_poiBanners.emplace_back(Banner::Type::Mopub, std::make_unique<Mopub>());

  m_searchBanners.emplace_back(Banner::Type::Facebook, std::make_unique<FacebookSearch>());

  m_downloadOnMapPromo.emplace_back(Banner::Type::BookmarkCatalog,
                                    std::make_unique<BookmarkCatalog>(*m_delegate));
  m_searchCategoryBanners.emplace_back(Banner::Type::Citymobil,
                                       std::make_unique<Citymobil>(*m_delegate));
}

std::vector<Banner> Engine::GetPoiBanners(feature::TypesHolder const & types,
                                          storage::CountriesVec const & countryIds,
                                          std::string const & userLanguage) const
{
  return GetBanners(m_poiBanners, m_delegate->IsAdsRemoved(), types, countryIds, userLanguage);
}

std::vector<Banner> Engine::GetSearchBanners() const
{
  return GetBanners(m_searchBanners, m_delegate->IsAdsRemoved());
}

std::vector<Banner> Engine::GetSearchCategoryBanners(std::optional<m2::PointD> const & userPos) const
{
  return GetBanners(m_searchCategoryBanners, m_delegate->IsAdsRemoved(), userPos);
}

std::vector<Banner> Engine::GetDownloadOnMapBanners(storage::CountryId const & downloadMwmId,
                                                    std::optional<m2::PointD> const & userPos,
                                                    std::string const & userLanguage) const
{
  auto result = GetBanners(m_downloadOnMapBanners, m_delegate->IsAdsRemoved(), downloadMwmId,
                           userPos, userLanguage);
  // Promo banners are not removable by "Remove Ads" subscription.
  auto promo = GetBanners(m_downloadOnMapPromo, false /* isAdsRemoved */, downloadMwmId, userPos,
                          userLanguage);
  for (auto & promoItem : promo)
  {
    result.emplace_back(std::move(promoItem));
  }

  return result;
}

void Engine::DisableAdProvider(Banner::Type const type, Banner::Place const place)
{
  switch (place)
  {
  case Banner::Place::Search: return SetAdProviderEnabled(m_searchBanners, type, false);
  case Banner::Place::Poi: return SetAdProviderEnabled(m_poiBanners, type, false);
  case Banner::Place::DownloadOnMap:
    return SetAdProviderEnabled(m_downloadOnMapBanners, type, false);
  case Banner::Place::SearchCategory:
    return SetAdProviderEnabled(m_searchCategoryBanners, type, false);
  }
}
}  // namespace ads
