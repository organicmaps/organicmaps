#pragma once

#include "std/unique_ptr.hpp"

namespace storage
{
class CountryInfoGetter;

unique_ptr<CountryInfoGetter> CreateCountryInfoGetter();
unique_ptr<CountryInfoGetter> CreateCountryInfoGetterMigrate();
} // namespace storage
