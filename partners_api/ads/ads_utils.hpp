#pragma once

#include "storage/storage_defines.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <unordered_set>

namespace ads
{
class WithSupportedLanguages
{
public:
  virtual ~WithSupportedLanguages() = default;

  void AppendSupportedUserLanguages(std::initializer_list<std::string> const & languages);
  bool IsLanguageSupported(std::string const & lang) const;

private:
  std::unordered_set<int8_t> m_supportedUserLanguages;
};

class WithSupportedCountries
{
public:
  virtual ~WithSupportedCountries() = default;

  void AppendSupportedCountries(std::initializer_list<storage::CountryId> const & countries);
  void AppendExcludedCountries(std::initializer_list<storage::CountryId> const & countries);
  bool IsCountrySupported(storage::CountryId const & countryId) const;
  bool IsCountryExcluded(storage::CountryId const & countryId) const;

private:
  // All countries are supported when empty.
  std::unordered_set<storage::CountryId> m_supportedCountries;
  std::unordered_set<storage::CountryId> m_excludedCountries;
};

class WithSupportedUserPos
{
public:
  virtual ~WithSupportedUserPos() = default;

  void AppendSupportedUserPosCountries(std::initializer_list<storage::CountryId> const & countries);
  void AppendExcludedUserPosCountries(std::initializer_list<storage::CountryId> const & countries);
  bool IsUserPosCountrySupported(storage::CountryId const & countryId) const;
  bool IsUserPosCountryExcluded(storage::CountryId const & countryId) const;

private:
  WithSupportedCountries m_countries;
};
}  // namespace ads
