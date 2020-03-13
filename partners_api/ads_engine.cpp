#include "partners_api/ads_engine.hpp"
#include "partners_api/facebook_ads.hpp"
#include "partners_api/google_ads.hpp"
#include "partners_api/mopub_ads.hpp"
#include "partners_api/rb_ads.hpp"

#include "indexer/feature_data.hpp"

#include <memory>

namespace ads
{
Engine::Engine()
{
  // The banner systems are placed by priority. First has a top priority.
  m_poiBanners.emplace_back(Banner::Type::RB, std::make_unique<Rb>());
  m_poiBanners.emplace_back(Banner::Type::Mopub, std::make_unique<Mopub>());

  m_searchBanners.emplace_back(Banner::Type::Facebook, std::make_unique<FacebookSearch>());

}

std::vector<Banner> Engine::GetPoiBanners(feature::TypesHolder const & types,
                                          storage::CountriesVec const & countryIds,
                                          std::string const & userLanguage) const
{
  std::vector<Banner> result;

  for (auto const & item : m_poiBanners)
  {
    if (!item.m_enabled)
      continue;

    for (auto const & countryId : countryIds)
    {
      auto const bannerId = item.m_container->GetBanner(types, countryId, userLanguage);
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

std::vector<Banner> Engine::GetSearchBanners() const
{
  std::vector<Banner> result;

  for (auto const & item : m_searchBanners)
  {
    auto const bannerId = item.m_container->GetBanner();

    if (!bannerId.empty())
      result.emplace_back(item.m_type, bannerId);
  }

  return result;
}

void Engine::DisableAdProvider(Banner::Type const type, Banner::Place const place)
{
  switch (place)
  {
  case Banner::Place::Search: return SetAdProviderEnabled(m_searchBanners, type, false);
  case Banner::Place::Poi: return SetAdProviderEnabled(m_poiBanners, type, false);
  }
}
}  // namespace ads
