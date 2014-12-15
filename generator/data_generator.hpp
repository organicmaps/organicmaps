#pragma once

#include "../std/string.hpp"

namespace data
{
  bool GenerateToFile(string const & dir, bool lightNodes, string const & osmFileName = string());
}
