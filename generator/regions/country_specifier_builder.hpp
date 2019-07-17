#pragma once

#include "generator/regions/country_specifier.hpp"

#include "base/logging.hpp"

#include <memory>
#include <string>

namespace generator
{
namespace regions
{
class CountrySpecifierBuilder
{
public:
  template <class T>
  void static Register()
  {
    for (std::string name : T::GetCountryNames())
      m_specifiers.emplace(std::make_pair(std::move(name), []() { return std::make_unique<T>(); }));
  }

  static std::unique_ptr<CountrySpecifier> GetCountrySpecifier(std::string const & country)
  {
    auto it = m_specifiers.find(country);
    if (it == m_specifiers.end())
    {
      LOG(LWARNING, ("Country", country, "has not specifier, look at generator/regions/specs"));
      return std::make_unique<CountrySpecifier>();
    }

    return it->second();
  }

private:
  static std::unordered_map<std::string, std::function<std::unique_ptr<CountrySpecifier>(void)>>
      m_specifiers;
  static std::set<std::string> kCountriesWithoutSpecifier;
  static std::set<std::string> kCountriesWithSpecifier;
  static std::mutex kMutex;
};

#define REGISTER_COUNTRY_SPECIFIER(Specifier)                                                   \
  struct REGISTER_COUNTRY_SPECIFIER##Specifier                                                  \
  {                                                                                             \
    REGISTER_COUNTRY_SPECIFIER##Specifier() { CountrySpecifierBuilder::Register<Specifier>(); } \
  };                                                                                            \
  static REGISTER_COUNTRY_SPECIFIER##Specifier a;

}  // namespace regions
}  // namespace generator
