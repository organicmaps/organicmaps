#include "country_specifier_builder.hpp"

#include "generator/regions/specs/rus.hpp"

namespace generator
{
namespace regions
{
std::unique_ptr<CountrySpecifier> GetCountrySpecifier(std::string const & countryName)
{
  if (countryName == u8"Russia")
    return std::make_unique<specs::RusSpecifier>();

  return std::make_unique<CountrySpecifier>();
}
}  // namespace regions
}  // namespace generator
