#include "partners_api/ads_base.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <algorithm>

namespace ads
{
Container::Container()
{
  AppendExcludedTypes({{"sponsored", "booking"},
                       {"tourism", "hotel"},
                       {"tourism", "apartment"},
                       {"tourism", "camp_site"},
                       {"tourism", "chalet"},
                       {"tourism", "guest_house"},
                       {"tourism", "hostel"},
                       {"tourism", "motel"},
                       {"tourism", "resort"}});
}

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

void Container::AppendSupportedUserLanguages(std::initializer_list<std::string> const & languages)
{
  for (auto const & language : languages)
  {
    int8_t const langIndex = StringUtf8Multilang::GetLangIndex(language);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      continue;
    m_supportedUserLanguages.insert(langIndex);
  }
}

bool Container::HasBanner(feature::TypesHolder const & types,
                          storage::TCountryId const & countryId,
                          std::string const & userLanguage) const
{
  if (!m_supportedCountries.empty() &&
      m_supportedCountries.find(countryId) == m_supportedCountries.end())
  {
    auto const userLangCode = StringUtf8Multilang::GetLangIndex(userLanguage);
    if (m_supportedUserLanguages.find(userLangCode) == m_supportedUserLanguages.end())
      return false;
  }
  return !m_excludedTypes.Contains(types);
}

std::string Container::GetBannerId(feature::TypesHolder const & types,
                                   storage::TCountryId const & countryId,
                                   std::string const & userLanguage) const
{
  if (!HasBanner(types, countryId, userLanguage))
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
