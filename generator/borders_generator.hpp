#pragma once

#include "../geometry/region2d.hpp"
#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

namespace osm
{
  void GenerateBordersFromOsm(string const & tagAndOptValue, string const & osmFile,
                              string const & outFile);
}
