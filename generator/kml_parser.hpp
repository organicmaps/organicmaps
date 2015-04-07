#pragma once

#include "std/string.hpp"

class PolygonsContainerT;

namespace kml
{
  bool LoadPolygons(string const & kmlFile, PolygonsContainerT & country, int level);
}
