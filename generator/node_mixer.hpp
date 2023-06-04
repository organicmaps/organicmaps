#pragma once

#include "generator/osm_element.hpp"

#include <fstream>
#include <functional>
#include <string>

namespace generator
{
void MixFakeNodes(std::istream & stream, std::function<void(OsmElement &)> const & processor);

inline void MixFakeNodes(std::string const filePath, std::function<void(OsmElement &)> const & processor)
{
  std::ifstream stream(filePath);
  MixFakeNodes(stream, processor);
}
}  // namespace generator
