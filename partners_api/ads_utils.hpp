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
protected:
  virtual ~WithSupportedLanguages() = default;

  void AppendSupportedUserLanguages(std::initializer_list<std::string> const & languages);
  bool IsLanguageSupported(std::string const & lang) const;

private:
  std::unordered_set<int8_t> m_supportedUserLanguages;
};

class WithSupportedCountries
{
protected:
  virtual ~WithSupportedCountries() = default;

  void AppendSupportedCountries(std::initializer_list<storage::CountryId> const & countries);
  bool IsCountrySupported(storage::CountryId const & countryId) const;

private:
  // All countries are supported when empty.
  std::unordered_set<storage::CountryId> m_supportedCountries;
};
}  // namespace ads
