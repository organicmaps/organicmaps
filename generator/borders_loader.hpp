#pragma once

#include "../geometry/region2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../std/string.hpp"

#include "country_loader.hpp"

namespace borders
{
  void GeneratePackedBorders(string const & baseDir);
}
