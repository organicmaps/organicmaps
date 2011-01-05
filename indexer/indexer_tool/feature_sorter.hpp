#pragma once

#include "../../std/string.hpp"

namespace feature
{
  /// Final generation of data from input feature-dat-file.
  /// @param[in] bSort sorts features in the given file by their mid points
  bool GenerateFinalFeatures(string const & datFile, bool bSort);
}
