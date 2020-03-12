#include "partners_api/ads_base.hpp"

#include "partners_api/promo_catalog_types.hpp"

#include "indexer/feature_data.hpp"

#include <algorithm>

namespace ads
{
PoiContainer::PoiContainer()
{
  AppendExcludedTypes({{"sponsored", "booking"},
                       {"tourism", "hotel"},
                       {"tourism", "apartment"},
                       {"tourism", "camp_site"},
                       {"tourism", "chalet"},
                       {"tourism", "guest_house"},
                       {"tourism", "hostel"},
                       {"tourism", "motel"},
                       {"tourism", "resort"},
                       {"sponsored", "promo_catalog"}});

  m_excludedTypes.Append(promo::GetPromoCatalogSightseeingsTypes());
  m_excludedTypes.Append(promo::GetPromoCatalogOutdoorTypes());
}

void PoiContainer::AppendEntry(std::initializer_list<std::initializer_list<char const *>> && types,
                               std::string const & id)
{
  m_typesToBanners.Append(std::move(types), id);
}

void PoiContainer::AppendExcludedTypes(
    std::initializer_list<std::initializer_list<char const *>> && types)
{
  m_excludedTypes.Append(std::move(types));
}

bool PoiContainer::HasBanner(feature::TypesHolder const & types,
                             storage::CountryId const & countryId,
                             std::string const & userLanguage) const
{
  return (IsCountrySupported(countryId) || IsLanguageSupported(userLanguage)) &&
         !m_excludedTypes.Contains(types);
}

std::string PoiContainer::GetBanner(feature::TypesHolder const & types,
                                    storage::CountryId const & countryId,
                                    std::string const & userLanguage) const
{
  if (!HasBanner(types, countryId, userLanguage))
    return {};

  auto const it = m_typesToBanners.Find(types);
  if (m_typesToBanners.IsValid(it))
    return it->second;

  return GetBannerForOtherTypes();
}

std::string PoiContainer::GetBannerForOtherTypes() const
{
  return {};
}
}  // namespace ads
