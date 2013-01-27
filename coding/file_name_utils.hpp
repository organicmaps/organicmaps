#pragma once

#include "../std/string.hpp"

namespace my
{
  /// Remove extension from file name.
  void GetNameWithoutExt(string & name);

  /// Get file name from full path.
  void GetNameFromFullPath(string & name);
}
