#include "country_specifier_builder.hpp"

#include "generator/regions/specs/rus.hpp"

namespace generator
{
namespace regions
{
std::unique_ptr<CountrySpecifier> GetCountrySpecifier(std::string const & countryName)
{
  if (countryName == u8"Россия" || countryName == u8"Российская Федерация" || countryName == u8"РФ")
    return std::make_unique<specs::RusSpecifier>();

  return std::make_unique<CountrySpecifier>();
}
}  // namespace regions
}  // namespace generator
