
#include "partners_api/ads/ads_base.hpp"

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

std::string PoiContainer::GetBanner(feature::TypesHolder const & types,
                                    storage::CountriesVec const & countryIds,
                                    std::string const & userLanguage) const
{
  if (!HasBanner(types, countryIds, userLanguage))
    return {};

  auto const it = m_typesToBanners.Find(types);
  if (m_typesToBanners.IsValid(it))
    return it->second;

  return GetBannerForOtherTypes();
}

std::string PoiContainer::GetBannerForOtherTypesForTesting() const
{
  return GetBannerForOtherTypes();
}

bool PoiContainer::HasBanner(feature::TypesHolder const & types,
                             storage::CountriesVec const & countryIds,
                             std::string const & userLanguage) const
{
  auto const isCountryExcluded =
      std::any_of(countryIds.begin(), countryIds.end(),
                  [this](auto const & id) { return IsCountryExcluded(id); });

  if (isCountryExcluded)
    return false;

  auto isCountrySupported = std::any_of(countryIds.begin(), countryIds.end(),
                                        [this](auto const & id) { return IsCountrySupported(id); });

  // When all countries are not supported - check user`s language.
  return (isCountrySupported || IsLanguageSupported(userLanguage)) &&
         !m_excludedTypes.Contains(types);
}

std::string PoiContainer::GetBannerForOtherTypes() const
{
  return {};
}

SearchCategoryContainerBase::SearchCategoryContainerBase(Delegate const & delegate)
  : m_delegate(delegate)
{
}

DownloadOnMapContainer::DownloadOnMapContainer(Delegate & delegate)
  : m_delegate(delegate)
{
}

std::string DownloadOnMapContainer::GetBanner(storage::CountryId const & countryId,
                                              std::optional<m2::PointD> const & userPos,
                                              std::string const & userLanguage) const
{
  if (!HasBanner(countryId, userPos, userLanguage))
    return {};

  return GetBannerInternal();
}

bool DownloadOnMapContainer::HasBanner(storage::CountryId const & countryId,
                                       std::optional<m2::PointD> const & userPos,
                                       std::string const & userLanguage) const
{
  storage::CountriesVec userPosCountries;
  if (userPos)
  {
    auto const userPosCountryId = m_delegate.GetCountryId(*userPos);
    userPosCountries.emplace_back(userPosCountryId);
    userPosCountries.emplace_back(m_delegate.GetTopmostParentFor(userPosCountryId));
  }

  bool isUserPosSupported =
      (userPosCountries.empty() && IsUserPosCountrySupported("")) ||
      std::any_of(userPosCountries.begin(), userPosCountries.end(),
                  [this](auto const & id) { return IsUserPosCountrySupported(id); });
  if (!isUserPosSupported)
    return false;

  storage::CountriesVec downloadMwmCountries;
  downloadMwmCountries.emplace_back(countryId);
  downloadMwmCountries.emplace_back(m_delegate.GetTopmostParentFor(countryId));

  return !std::any_of(userPosCountries.begin(), userPosCountries.end(),
                      [this](auto const & id) { return IsUserPosCountryExcluded(id); }) &&
         !std::any_of(downloadMwmCountries.begin(), downloadMwmCountries.end(),
                      [this](auto const & id) { return IsCountryExcluded(id); }) &&
         std::any_of(downloadMwmCountries.begin(), downloadMwmCountries.end(),
                     [this](auto const & id) { return IsCountrySupported(id); }) &&
         IsLanguageSupported(userLanguage);
}

std::string DownloadOnMapContainer::GetBannerInternal() const
{
  return {};
}
}  // namespace ads
