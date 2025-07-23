#pragma once

#include "geometry/mercator.hpp"

#include <memory>

namespace storage
{
class CountryInfoGetter;

std::unique_ptr<CountryInfoGetter> CreateCountryInfoGetter();

}  // namespace storage
