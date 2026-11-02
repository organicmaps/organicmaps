#pragma once

#include "storage/storage_defines.hpp"

#include "platform/get_text_by_id.hpp"

#include <memory>
#include <string>

namespace storage
{
/// Looks up country names in a specified locale, falling back to English if the locale file cannot be opened.
/// If a country name is missing or empty, operator() returns the country ID.
class CountryNameGetter
{
public:
  /// @return current locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note The method returns correct locale after SetLocale has been called.
  /// If not, it returns an empty string.
  std::string GetLocale() const;

  /// \brief Sets a locale.
  /// @param locale is a string representation of locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note See data/countries-strings/ for the available locales.
  void SetLocale(std::string const & locale);

  /// \brief Gets localized string for key. If not found returns empty string.
  /// @param key is a string for lookup.
  /// \note Unlike operator (), does not return key if localized string is not found.
  std::string Get(std::string const & key) const;

  std::string operator()(CountryId const & countryId) const;

  // for testing
  void SetLocaleForTesting(std::string const & jsonBuffer, std::string const & locale);

private:
  std::unique_ptr<platform::GetTextById> m_getCurLang;
};
}  // namespace storage
