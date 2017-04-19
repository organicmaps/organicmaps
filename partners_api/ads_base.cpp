#include "partners_api/ads_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include <algorithm>

namespace ads
{
Container::Container() { AppendExcludedTypes({{"sponsored", "booking"}}); }

void Container::AppendEntry(std::initializer_list<std::initializer_list<char const *>> && types,
                            std::string const & id)
{
  m_typesToBanners.Append(std::move(types), id);
}

void Container::AppendExcludedTypes(
    std::initializer_list<std::initializer_list<char const *>> && types)
{
  m_excludedTypes.Append(std::move(types));
}

void Container::AppendSupportedCountries(
    std::initializer_list<storage::TCountryId> const & countries)
{
  m_supportedCountries.insert(countries.begin(), countries.end());
}

bool Container::HasBanner(feature::TypesHolder const & types,
                          storage::TCountryId const & countryId) const
{
  if (!m_supportedCountries.empty() &&
      m_supportedCountries.find(countryId) == m_supportedCountries.end())
  {
    return false;
  }
  return !m_excludedTypes.Contains(types);
}

std::string Container::GetBannerId(feature::TypesHolder const & types,
                                   storage::TCountryId const & countryId) const
{
  if (!HasBanner(types, countryId))
    return {};

  auto const it = m_typesToBanners.Find(types);
  if (m_typesToBanners.IsValid(it))
    return it->second;

  return GetBannerIdForOtherTypes();
}

std::string Container::GetBannerIdForOtherTypes() const
{
  return {};
}

bool Container::HasSearchBanner() const
{
  return false;
}

std::string Container::GetSearchBannerId() const
{
  return {};
}
}  // namespace ads
