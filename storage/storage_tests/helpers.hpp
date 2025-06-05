#pragma once

#include "geometry/mercator.hpp"

#include <memory>

namespace storage
{
class CountryInfoGetter;

std::unique_ptr<CountryInfoGetter> CreateCountryInfoGetter();

bool AlmostEqualRectsAbs(m2::RectD const & r1, m2::RectD const & r2);
}  // namespace storage
