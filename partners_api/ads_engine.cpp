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
  m_banners.emplace_back(Banner::Type::RB, std::make_unique<Rb>());
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
      if (item.m_enabled && item.m_container->HasBanner(types, countryId, userLanguage))
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
    if (!item.m_enabled)
      continue;

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
    if (item.m_enabled && item.m_container->HasSearchBanner())
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

void Engine::DisableAdProvider(Banner::Type const type, Banner::Place const place)
{
  SetAdProviderEnabled(place == Banner::Place::Search ? m_searchBanners : m_banners, type, false);
}

void Engine::SetAdProviderEnabled(std::vector<Engine::ContainerItem> & banners,
                                  Banner::Type const type, bool const isEnabled)
{
  for (auto & item : banners)
  {
    if (item.m_type == type)
    {
      item.m_enabled = isEnabled;
      return;
    }
  }
}
}  // namespace ads
