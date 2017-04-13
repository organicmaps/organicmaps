#include "partners_api/ads_engine.hpp"
#include "partners_api/mopub_ads.hpp"
#include "partners_api/rb_ads.hpp"

#include "indexer/feature_data.hpp"

#include "base/stl_add.hpp"

#include <algorithm>

namespace ads
{
Engine::Engine()
{
  // The banner systems are placed by priority. First has a top priority.
  m_containers.emplace_back(Banner::Type::RB, my::make_unique<Rb>());
  m_containers.emplace_back(Banner::Type::Mopub, my::make_unique<Mopub>());
}

bool Engine::HasBanner(feature::TypesHolder const & types,
                       storage::TCountriesVec const & countryIds) const
{
  for (auto const & countryId : countryIds)
  {
    for (auto const & item : m_containers)
    {
      if (item.m_container->HasBanner(types, countryId))
        return true;
    }
  }

  return false;
}

std::vector<Banner> Engine::GetBanners(feature::TypesHolder const & types,
                                       storage::TCountriesVec const & countryIds) const
{
  std::vector<Banner> banners;

  for (auto const & item : m_containers)
  {
    for (auto const & countryId : countryIds)
    {
      auto const bannerId = item.m_container->GetBannerId(types, countryId);
      // We need to add banner for every banner system just once.
      if (!bannerId.empty())
      {
        banners.emplace_back(item.m_type, bannerId);
        break;
      }
    }
  }

  return banners;
}
}  // namespace ads
