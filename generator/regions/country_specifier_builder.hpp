#pragma once

#include "generator/regions/country_specifier.hpp"

#include <memory>
#include <string>

namespace generator
{
namespace regions
{
std::unique_ptr<CountrySpecifier> GetCountrySpecifier(std::string const & countryName);
}  // namespace regions
}  // namespace generator
