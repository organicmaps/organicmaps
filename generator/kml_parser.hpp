#pragma once

#include "generator/std/string.hpp"

class PolygonsContainerT;

namespace kml
{
  bool LoadPolygons(std::string const & kmlFile, PolygonsContainerT & country, int level);
}
