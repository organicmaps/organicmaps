#include "partners_api/ads/ads_utils.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace ads
{
void WithSupportedLanguages::AppendSupportedUserLanguages(
    std::initializer_list<std::string> const & languages)
{
  for (auto const & language : languages)
  {
    int8_t const langIndex = StringUtf8Multilang::GetLangIndex(language);
    if (langIndex == StringUtf8Multilang::kUnsupportedLanguageCode)
      continue;
    m_supportedUserLanguages.insert(langIndex);
  }
}

bool WithSupportedLanguages::IsLanguageSupported(std::string const & lang) const
{
  auto const userLangCode = StringUtf8Multilang::GetLangIndex(lang);
  return m_supportedUserLanguages.find(userLangCode) != m_supportedUserLanguages.end();
}

void WithSupportedCountries::AppendSupportedCountries(
    std::initializer_list<storage::CountryId> const & countries)
{
  m_supportedCountries.insert(countries.begin(), countries.end());
}

bool WithSupportedCountries::IsCountrySupported(storage::CountryId const & countryId) const
{
  return m_supportedCountries.empty() ||
         m_supportedCountries.find(countryId) != m_supportedCountries.cend();
}
}  // namespace ads
