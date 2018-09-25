#include "partners_api/ads_engine.hpp"
#include "partners_api/facebook_ads.hpp"
#include "partners_api/google_ads.hpp"
#include "partners_api/mopub_ads.hpp"
#include "partners_api/rb_ads.hpp"

#include "indexer/feature_data.hpp"

#include <algorithm>
#include <memory>

namespace ads
{
Engine::Engine()
{
  // The banner systems are placed by priority. First has a top priority.
  m_banners.emplace_back(Banner::Type::Mopub, std::make_unique<Mopub>());

  m_searchBanners.emplace_back(Banner::Type::Facebook, std::make_unique<Facebook>());
}

bool Engine::HasBanner(feature::TypesHolder const & types,
                       storage::TCountriesVec const & countryIds,
                       std::string const & userLanguage) const
{
  for (auto const & countryId : countryIds)
  {
    for (auto const & item : m_banners)
    {
      if (item.m_container->HasBanner(types, countryId, userLanguage))
        return true;
    }
  }

  return false;
}

std::vector<Banner> Engine::GetBanners(feature::TypesHolder const & types,
                                       storage::TCountriesVec const & countryIds,
                                       std::string const & userLanguage) const
{
  std::vector<Banner> result;

  for (auto const & item : m_banners)
  {
    for (auto const & countryId : countryIds)
    {
      auto const bannerId = item.m_container->GetBannerId(types, countryId, userLanguage);
      // We need to add banner for every banner system just once.
      if (!bannerId.empty())
      {
        result.emplace_back(item.m_type, bannerId);
        break;
      }
    }
  }

  return result;
}

bool Engine::HasSearchBanner() const
{
  for (auto const & item : m_searchBanners)
  {
    if (item.m_container->HasSearchBanner())
      return true;
  }

  return false;
}

std::vector<Banner> Engine::GetSearchBanners() const
{
  std::vector<Banner> result;

  for (auto const & item : m_searchBanners)
  {
    auto const bannerId = item.m_container->GetSearchBannerId();

    if (!bannerId.empty())
      result.emplace_back(item.m_type, bannerId);
  }

  return result;
}

void Engine::ExcludeAdProvider(Banner::Type const type)
{
  ExcludeAdProviderInternal(m_banners, type);
  ExcludeAdProviderInternal(m_searchBanners, type);
}

void Engine::IncludeAdProvider(Banner::Type const type, Banner::Place place)
{
  IncludeAdProviderInternal(place == Banner::Place::Search ? m_searchBanners : m_banners, type);
}

void Engine::ExcludeAdProviderInternal(std::vector<Engine::ContainerItem> & banners, Banner::Type const type)
{
  banners.erase(std::remove_if(banners.begin(), banners.end(), [type](ContainerItem const & each)
  {
    return each.m_type == type;
  }), banners.end());
}

void Engine::IncludeAdProviderInternal(std::vector<ContainerItem> & banners, Banner::Type const type)
{
  auto const hasItemIterator = std::find_if(std::begin(banners), std::end(banners), [type](ContainerItem const & each)
  {
    return each.m_type == type;
  });

  if (hasItemIterator != std::end(banners))
    return;

  switch (type)
  {
      case Banner::Type::Facebook :
          banners.emplace_back(Banner::Type::Facebook, std::make_unique<Facebook>());
          break;
      case Banner::Type::RB :
          banners.emplace_back(Banner::Type::RB, std::make_unique<Rb>());
          break;
      case Banner::Type::Mopub :
          banners.emplace_back(Banner::Type::Mopub, std::make_unique<Mopub>());
          break;
      case Banner::Type::Google :
          banners.emplace_back(Banner::Type::None, std::make_unique<Google>());
          break;
      case Banner::Type::None :
          break;
  }
}
}  // namespace ads
