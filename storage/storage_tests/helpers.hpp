#pragma once

#include "geometry/mercator.hpp"

#include "std/unique_ptr.hpp"

namespace storage
{
class CountryInfoGetter;

unique_ptr<CountryInfoGetter> CreateCountryInfoGetter();
unique_ptr<CountryInfoGetter> CreateCountryInfoGetterMigrate();

bool AlmostEqualRectsAbs(const m2::RectD & r1, const m2::RectD & r2);
} // namespace storage
