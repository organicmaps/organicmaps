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
  static CountrySpecifierBuilder & GetInstance()
  {
    static CountrySpecifierBuilder instance;
    return instance;
  }

  std::unique_ptr<CountrySpecifier> MakeCountrySpecifier(std::string const & country) const;

  template <class T>
  void Register()
  {
    for (std::string name : T::GetCountryNames())
      m_specifiers.emplace(std::make_pair(std::move(name), []() { return std::make_unique<T>(); }));
  }

private:
  CountrySpecifierBuilder() = default;
  CountrySpecifierBuilder(const CountrySpecifierBuilder&) = delete;
  CountrySpecifierBuilder& operator=(const CountrySpecifierBuilder&) = delete;
  CountrySpecifierBuilder(CountrySpecifierBuilder&&) = delete;
  CountrySpecifierBuilder& operator=(CountrySpecifierBuilder&&) = delete;

  std::unordered_map<std::string, std::function<std::unique_ptr<CountrySpecifier>(void)>>
      m_specifiers;
};

#define REGISTER_COUNTRY_SPECIFIER(Specifier)                                                   \
  struct COUNTRY_SPECIFIER_##Specifier                                                          \
  {                                                                                             \
    COUNTRY_SPECIFIER_##Specifier()                                                    \
    {                                                                                           \
      CountrySpecifierBuilder::GetInstance().Register<Specifier>();                             \
    }                                                                                           \
  };                                                                                            \
  static COUNTRY_SPECIFIER_##Specifier STATIC_VAR_COUNTRY_SPECIFIER_##Specifier;


}  // namespace regions
}  // namespace generator
