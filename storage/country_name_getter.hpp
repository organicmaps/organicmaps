#pragma once

#include "storage/index.hpp"

#include "platform/get_text_by_id.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace storage
{

/// CountryNameGetter is responsible for generating text name for TCountryId in a specified locale
/// To get this country name use operator().
/// If the country caption is not available for specified locale, class tries to find it in
/// English locale.
class CountryNameGetter
{
public:
  /// @return current locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note The method returns correct locale after SetLocale has been called.
  /// If not, it returns an empty string.
  string GetLocale() const;

  /// \brief Sets a locale.
  /// @param locale is a string representation of locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note See countries/languages.txt for the full list of available locales.
  void SetLocale(string const & locale);

  /// \brief Gets localized string for key. If not found returns empty string.
  /// @param key is a string for lookup.
  /// \note Unlike operator (), does not return key if localized string is not found.
  string Get(string const & key) const;

  string operator()(TCountryId const & countryId) const;

  // for testing
  void SetLocaleForTesting(string const & jsonBuffer, string const & locale);

private:
  unique_ptr<platform::GetTextById> m_getCurLang;
};

}  // namespace storage
