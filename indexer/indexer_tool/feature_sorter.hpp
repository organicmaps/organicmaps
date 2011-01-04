#pragma once

#include "../../std/string.hpp"

namespace feature
{
  // sorts features in the given file by their mid points
  bool SortDatFile(string const & datFile, bool removeOriginalFile = true);
}
